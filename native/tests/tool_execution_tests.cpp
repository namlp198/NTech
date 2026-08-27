#include "nvision/acquisition/frame.hpp"
#include "nvision/acquisition/image_view.hpp"
#include "nvision/core/builtin_catalog.hpp"
#include "nvision/core/parameter_schema.hpp"
#include "nvision/core/tool_definition.hpp"
#include "nvision/core/tool_registry.hpp"
#include "nvision/core/workflow_node.hpp"
#include "nvision/vision/builtin_tool_implementations.hpp"
#include "nvision/workflow/port_value.hpp"
#include "nvision/workflow/tool_execution.hpp"
#include "nvision/workflow/tool_implementation_registry.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#define NVISION_REQUIRE(condition) \
    do {                           \
        if (!(condition)) {        \
            std::abort();          \
        }                          \
    } while (false)

namespace {

using nvision::acquisition::Frame;
using nvision::acquisition::FrameBuffer;
using nvision::acquisition::FrameLease;
using nvision::acquisition::FrameMetadata;
using nvision::acquisition::FrameStatus;
using nvision::acquisition::ImageView;
using nvision::acquisition::PixelFormat;
using nvision::acquisition::TimestampQuality;
using nvision::core::AxisAlignedRectangle;
using nvision::core::Circle;
using nvision::core::create_builtin_registry;
using nvision::core::ErrorCategory;
using nvision::core::ExecutionError;
using nvision::core::ExecutionSessionId;
using nvision::core::ExecutionStatus;
using nvision::core::NodeConfiguration;
using nvision::core::NodeParameterAssignment;
using nvision::core::ParameterDefinition;
using nvision::core::ParameterKey;
using nvision::core::ParameterType;
using nvision::core::ParameterValue;
using nvision::core::Point2D;
using nvision::core::PortDataType;
using nvision::core::PortDirection;
using nvision::core::PortKey;
using nvision::core::ResultCode;
using nvision::core::SemanticVersion;
using nvision::core::ToolCatalogMetadata;
using nvision::core::ToolPortDefinition;
using nvision::core::ToolRegistry;
using nvision::core::ToolTypeDefinition;
using nvision::core::ToolTypeId;
using nvision::core::WorkflowNodeId;
using nvision::vision::create_vision_implementation_registry;
using nvision::workflow::CancellationSource;
using nvision::workflow::execute_tool;
using nvision::workflow::ExecutionBudget;
using nvision::workflow::ParameterBinding;
using nvision::workflow::PortValue;
using nvision::workflow::PortValueMap;
using nvision::workflow::ToolExecutionContext;
using nvision::workflow::ToolExecutionOutcome;
using nvision::workflow::ToolImplementation;
using nvision::workflow::ToolImplementationRegistry;
using nvision::workflow::ToolImplementationStatus;

template <typename Action>
bool rejects_invalid_argument(Action action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

const SemanticVersion& initial_version() {
    static const SemanticVersion version(1U, 0U, 0U);
    return version;
}

// A 4x3 mono8 frame whose pixel value encodes its own position, so a crop can be
// checked against the exact source pixels rather than against a size alone.
ImageView sample_image() {
    std::vector<std::uint8_t> pixels(12U);
    for (std::uint32_t y = 0U; y < 3U; ++y) {
        for (std::uint32_t x = 0U; x < 4U; ++x) {
            pixels[static_cast<std::size_t>(y) * 4U + x] = static_cast<std::uint8_t>(y * 10U + x);
        }
    }
    FrameLease frame = std::make_shared<const Frame>(
        std::array<std::uint8_t, 16>{7U},
        "test-camera",
        1U,
        0,
        0,
        4U,
        3U,
        4U,
        PixelFormat::mono8,
        std::make_shared<const FrameBuffer>(std::move(pixels)),
        FrameStatus::complete,
        TimestampQuality::host_only,
        FrameMetadata{});
    return ImageView(std::move(frame));
}

ToolTypeDefinition counting_definition() {
    return ToolTypeDefinition(
        ToolTypeId("nvision.test.tool.counter"),
        initial_version(),
        ToolCatalogMetadata("Counter", {"Test"}),
        {ToolPortDefinition(
             PortKey("image"),
             "Image",
             PortDirection::input,
             PortDataType::image_view,
             true),
         ToolPortDefinition(
             PortKey("threshold"),
             "Threshold",
             PortDirection::input,
             PortDataType::integer,
             false),
         ToolPortDefinition(
             PortKey("count"),
             "Count",
             PortDirection::output,
             PortDataType::integer,
             true)},
        nvision::core::ToolOrigin::built_in(),
        {},
        {ParameterDefinition(
             ParameterKey("scale"),
             "Scale",
             ParameterType::floating_point(),
             true,
             ParameterValue::floating_point(1.0)),
         ParameterDefinition(
             ParameterKey("label"),
             "Label",
             ParameterType::string(),
             true)});
}

// One behaviour per instance keeps the fake honest: each test states exactly which
// contract violation or failure mode it exercises.
enum class FakeBehaviour : std::uint32_t {
    produce_declared_output,
    produce_undeclared_output,
    produce_wrong_output_kind,
    produce_nothing,
    raise,
    sleep_then_succeed,
};

class FakeTool final : public ToolImplementation {
public:
    explicit FakeTool(const FakeBehaviour behaviour)
        : behaviour_(behaviour), id_("nvision.test.tool.counter"), version_(1U, 0U, 0U) {}

    [[nodiscard]] const ToolTypeId& tool_type_id() const noexcept override {
        return id_;
    }

    [[nodiscard]] const SemanticVersion& tool_version() const noexcept override {
        return version_;
    }

    [[nodiscard]] ToolExecutionOutcome execute(const ToolExecutionContext& context) const override {
        PortValueMap outputs;
        switch (behaviour_) {
            case FakeBehaviour::produce_declared_output:
                outputs.set(
                    PortKey("count"),
                    PortValue::integer(
                        static_cast<std::int64_t>(context.input(PortKey("image"))
                                                      .image_value()
                                                      .width())));
                return ToolExecutionOutcome::succeeded(std::move(outputs));
            case FakeBehaviour::produce_undeclared_output:
                outputs.set(PortKey("count"), PortValue::integer(1));
                outputs.set(PortKey("surprise"), PortValue::integer(2));
                return ToolExecutionOutcome::succeeded(std::move(outputs));
            case FakeBehaviour::produce_wrong_output_kind:
                outputs.set(PortKey("count"), PortValue::text("not a number"));
                return ToolExecutionOutcome::succeeded(std::move(outputs));
            case FakeBehaviour::produce_nothing:
                return ToolExecutionOutcome::succeeded({});
            case FakeBehaviour::sleep_then_succeed:
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                outputs.set(PortKey("count"), PortValue::integer(1));
                return ToolExecutionOutcome::succeeded(std::move(outputs));
            case FakeBehaviour::raise:
            default:
                throw std::runtime_error("the tool failed while reading its device");
        }
    }

private:
    FakeBehaviour behaviour_;
    ToolTypeId id_;
    SemanticVersion version_;
};

NodeConfiguration counter_configuration() {
    return NodeConfiguration(
        {NodeParameterAssignment(ParameterKey("label"), ParameterValue::string("primary"))});
}

ToolExecutionContext counter_context(
    const std::shared_ptr<const ToolTypeDefinition>& definition,
    PortValueMap inputs) {
    return ToolExecutionContext(
        definition,
        ExecutionSessionId("session.01"),
        WorkflowNodeId("node.01"),
        std::move(inputs),
        ParameterBinding::resolve(*definition, counter_configuration()));
}

PortValueMap counter_inputs() {
    PortValueMap inputs;
    inputs.set(PortKey("image"), PortValue::image(sample_image()));
    return inputs;
}

void typed_port_values_enforce_the_declared_data_kind() {
    const PortValue count = PortValue::integer(4);
    NVISION_REQUIRE(count.type() == PortDataType::integer);
    NVISION_REQUIRE(count.integer_value() == 4);
    NVISION_REQUIRE(rejects_invalid_argument([&count] { (void)count.image_value(); }));

    const PortValue image = PortValue::image(sample_image());
    NVISION_REQUIRE(image.type() == PortDataType::image_view);
    NVISION_REQUIRE(image.image_value().width() == 4U);

    // The same payload can carry a different contract, and the contract is what a port
    // is validated against.
    const PortValue mask = PortValue::mask(sample_image());
    NVISION_REQUIRE(mask.type() == PortDataType::mask);

    const auto definition = std::make_shared<const ToolTypeDefinition>(counting_definition());

    NVISION_REQUIRE(rejects_invalid_argument([&definition] {
        PortValueMap inputs = counter_inputs();
        inputs.set(PortKey("unknown"), PortValue::integer(1));
        (void)counter_context(definition, std::move(inputs));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&definition] {
        PortValueMap inputs;
        inputs.set(PortKey("image"), PortValue::text("not an image"));
        (void)counter_context(definition, std::move(inputs));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&definition] {
        (void)counter_context(definition, PortValueMap{});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&definition] {
        // An output port is not an input port, even when the key matches nothing else.
        PortValueMap inputs = counter_inputs();
        inputs.set(PortKey("count"), PortValue::integer(1));
        (void)counter_context(definition, std::move(inputs));
    }));

    // An optional input may be absent, and a declared optional input is accepted.
    const ToolExecutionContext without_optional = counter_context(definition, counter_inputs());
    NVISION_REQUIRE(without_optional.inputs().size() == 1U);
    PortValueMap with_optional = counter_inputs();
    with_optional.set(PortKey("threshold"), PortValue::integer(9));
    const ToolExecutionContext complete = counter_context(definition, std::move(with_optional));
    NVISION_REQUIRE(complete.input(PortKey("threshold")).integer_value() == 9);
}

void parameter_binding_resolves_defaults_and_rejects_incomplete_configuration() {
    const ToolTypeDefinition definition = counting_definition();

    const ParameterBinding binding =
        ParameterBinding::resolve(definition, counter_configuration());
    NVISION_REQUIRE(binding.size() == 2U);
    NVISION_REQUIRE(binding.value(ParameterKey("scale")).floating_point_value() == 1.0);
    NVISION_REQUIRE(binding.value(ParameterKey("label")).string_value() == "primary");

    // An assignment wins over the declared default.
    const ParameterBinding overridden = ParameterBinding::resolve(
        definition,
        NodeConfiguration(
            {NodeParameterAssignment(ParameterKey("label"), ParameterValue::string("secondary")),
             NodeParameterAssignment(
                 ParameterKey("scale"), ParameterValue::floating_point(2.5))}));
    NVISION_REQUIRE(overridden.value(ParameterKey("scale")).floating_point_value() == 2.5);

    // A key the bound version does not declare stays on the node but never reaches the
    // tool, so an implementation cannot read a value it did not declare.
    const ParameterBinding preserved = ParameterBinding::resolve(
        definition,
        NodeConfiguration(
            {NodeParameterAssignment(ParameterKey("label"), ParameterValue::string("primary")),
             NodeParameterAssignment(ParameterKey("legacy"), ParameterValue::integer(3))}));
    NVISION_REQUIRE(preserved.size() == 2U);
    NVISION_REQUIRE(preserved.find(ParameterKey("legacy")) == nullptr);

    NVISION_REQUIRE(rejects_invalid_argument([&definition] {
        (void)ParameterBinding::resolve(definition, NodeConfiguration{});
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&definition] {
        (void)ParameterBinding::resolve(
            definition,
            NodeConfiguration({NodeParameterAssignment(
                ParameterKey("label"), ParameterValue::integer(1))}));
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&binding] {
        (void)binding.value(ParameterKey("missing"));
    }));
}

void tool_outcomes_never_present_a_failure_as_a_result() {
    // A non-success outcome has no way to carry outputs: the factories that build one
    // take an error and nothing else, so a downstream node can never consume a value
    // produced by a tool that did not complete.
    const ToolExecutionOutcome failed = ToolExecutionOutcome::failed(ExecutionError(
        ResultCode("DEVICE_FAULT"), ErrorCategory::device, "device fault", false));
    NVISION_REQUIRE(failed.status() == ExecutionStatus::error);
    NVISION_REQUIRE(!failed.is_success());
    NVISION_REQUIRE(failed.outputs().empty());
    NVISION_REQUIRE(failed.errors().size() == 1U);

    const auto definition = std::make_shared<const ToolTypeDefinition>(counting_definition());
    const ToolExecutionContext context = counter_context(definition, counter_inputs());

    const FakeTool working(FakeBehaviour::produce_declared_output);
    const ToolExecutionOutcome success = execute_tool(working, context);
    NVISION_REQUIRE(success.is_success());
    NVISION_REQUIRE(success.outputs().find(PortKey("count"))->integer_value() == 4);

    const FakeTool raising(FakeBehaviour::raise);
    const ToolExecutionOutcome raised = execute_tool(raising, context);
    NVISION_REQUIRE(raised.status() == ExecutionStatus::error);
    NVISION_REQUIRE(raised.errors().front().code() == ResultCode("TOOL_EXECUTION_FAULT"));
    NVISION_REQUIRE(raised.outputs().empty());

    const FakeTool undeclared(FakeBehaviour::produce_undeclared_output);
    NVISION_REQUIRE(
        execute_tool(undeclared, context).errors().front().code() ==
        ResultCode("OUTPUT_CONTRACT_VIOLATION"));

    const FakeTool wrong_kind(FakeBehaviour::produce_wrong_output_kind);
    NVISION_REQUIRE(
        execute_tool(wrong_kind, context).errors().front().code() ==
        ResultCode("OUTPUT_CONTRACT_VIOLATION"));

    const FakeTool empty(FakeBehaviour::produce_nothing);
    NVISION_REQUIRE(
        execute_tool(empty, context).errors().front().code() ==
        ResultCode("OUTPUT_CONTRACT_VIOLATION"));
}

void cancellation_and_budget_are_observable_through_the_execution_contract() {
    const auto definition = std::make_shared<const ToolTypeDefinition>(counting_definition());

    CancellationSource source;
    source.request_cancellation();
    const ToolExecutionContext cancelled_context(
        definition,
        ExecutionSessionId("session.01"),
        WorkflowNodeId("node.01"),
        counter_inputs(),
        ParameterBinding::resolve(*definition, counter_configuration()),
        source.token());
    NVISION_REQUIRE(cancelled_context.is_cancellation_requested());

    const FakeTool working(FakeBehaviour::produce_declared_output);
    const ToolExecutionOutcome cancelled = execute_tool(working, cancelled_context);
    NVISION_REQUIRE(cancelled.status() == ExecutionStatus::cancelled);
    NVISION_REQUIRE(cancelled.errors().front().code() == ResultCode("EXECUTION_CANCELLED"));
    NVISION_REQUIRE(cancelled.outputs().empty());

    const ToolExecutionContext budgeted(
        definition,
        ExecutionSessionId("session.01"),
        WorkflowNodeId("node.01"),
        counter_inputs(),
        ParameterBinding::resolve(*definition, counter_configuration()),
        nvision::workflow::CancellationToken::none(),
        ExecutionBudget::of(std::chrono::milliseconds(1)));
    NVISION_REQUIRE(budgeted.budget().is_bounded());

    const FakeTool slow(FakeBehaviour::sleep_then_succeed);
    const ToolExecutionOutcome timed_out = execute_tool(slow, budgeted);
    NVISION_REQUIRE(timed_out.status() == ExecutionStatus::timeout);
    NVISION_REQUIRE(timed_out.errors().front().code() == ResultCode("EXECUTION_TIMEOUT"));
    NVISION_REQUIRE(timed_out.outputs().empty());

    NVISION_REQUIRE(!ExecutionBudget::unbounded().is_bounded());
    NVISION_REQUIRE(!ExecutionBudget::unbounded().is_exceeded_by(std::chrono::hours(1)));
    NVISION_REQUIRE(rejects_invalid_argument([] {
        (void)ExecutionBudget::of(std::chrono::nanoseconds::zero());
    }));
}

void the_implementation_registry_reports_unimplemented_definitions_explicitly() {
    const ToolRegistry definitions = create_builtin_registry();
    const ToolImplementationRegistry implementations = create_vision_implementation_registry();

    const ToolTypeId crop("nvision.tool.processing.crop");
    const auto bound = implementations.bind(crop, initial_version(), definitions);
    NVISION_REQUIRE(bound.status() == ToolImplementationStatus::bound);
    NVISION_REQUIRE(bound.is_bound());
    NVISION_REQUIRE(bound.definition()->id() == crop);

    // A catalogued tool without an implementation is not a missing tool.
    const auto pending = implementations.bind(
        ToolTypeId("nvision.tool.processing.resize"), initial_version(), definitions);
    NVISION_REQUIRE(pending.status() == ToolImplementationStatus::not_implemented);
    NVISION_REQUIRE(pending.definition() != nullptr);
    NVISION_REQUIRE(pending.implementation() == nullptr);

    const auto unknown = implementations.bind(
        ToolTypeId("nvision.tool.processing.absent"), initial_version(), definitions);
    NVISION_REQUIRE(unknown.status() == ToolImplementationStatus::unknown_tool);
    NVISION_REQUIRE(unknown.definition() == nullptr);

    const auto wrong_version =
        implementations.bind(crop, SemanticVersion(2U, 0U, 0U), definitions);
    NVISION_REQUIRE(wrong_version.status() == ToolImplementationStatus::unsupported_version);

    NVISION_REQUIRE(
        implementations.unimplemented(definitions).size() ==
        definitions.definitions().size() - implementations.size());

    ToolImplementationRegistry duplicate_check;
    nvision::vision::register_vision_tool_implementations(duplicate_check);
    NVISION_REQUIRE(rejects_invalid_argument([&duplicate_check] {
        nvision::vision::register_vision_tool_implementations(duplicate_check);
    }));
    NVISION_REQUIRE(rejects_invalid_argument([&duplicate_check] {
        duplicate_check.register_implementation(nullptr);
    }));
}

ToolExecutionContext crop_context(
    const std::shared_ptr<const ToolTypeDefinition>& definition,
    const ImageView& image,
    ParameterValue roi) {
    PortValueMap inputs;
    inputs.set(PortKey("image"), PortValue::image(image));
    return ToolExecutionContext(
        definition,
        ExecutionSessionId("session.01"),
        WorkflowNodeId("node.crop"),
        std::move(inputs),
        ParameterBinding::resolve(
            *definition,
            NodeConfiguration({NodeParameterAssignment(ParameterKey("roi"), std::move(roi))})));
}

void crop_executes_through_the_catalog_contract_without_copying_pixels() {
    const ToolRegistry definitions = create_builtin_registry();
    const ToolImplementationRegistry implementations = create_vision_implementation_registry();
    const auto binding = implementations.bind(
        ToolTypeId("nvision.tool.processing.crop"), initial_version(), definitions);
    NVISION_REQUIRE(binding.is_bound());

    const ImageView image = sample_image();
    const ParameterValue roi = ParameterValue::region_of_interest(
        AxisAlignedRectangle(image.coordinate_space(), 1.0, 1.0, 2.0, 2.0));

    const ToolExecutionOutcome outcome =
        execute_tool(*binding.implementation(), crop_context(binding.definition(), image, roi));
    NVISION_REQUIRE(outcome.is_success());
    NVISION_REQUIRE(outcome.outputs().size() == 2U);

    const ImageView& cropped = outcome.outputs().find(PortKey("image"))->image_value();
    NVISION_REQUIRE(cropped.width() == 2U);
    NVISION_REQUIRE(cropped.height() == 2U);
    // Row 0 of the crop is source row 1 starting at column 1: values 11 and 12.
    NVISION_REQUIRE(cropped.row(0U)[0] == 11U);
    NVISION_REQUIRE(cropped.row(0U)[1] == 12U);
    NVISION_REQUIRE(cropped.row(1U)[0] == 21U);
    // The crop retains the same frame rather than materializing a new image.
    NVISION_REQUIRE(cropped.frame().get() == image.frame().get());

    const auto& transform = outcome.outputs().find(PortKey("transform"))->transform_value();
    NVISION_REQUIRE(transform.source_space() == cropped.coordinate_space());
    NVISION_REQUIRE(transform.target_space() == image.source_coordinate_space());
    const Point2D mapped = transform.apply(Point2D(cropped.coordinate_space(), 0.0, 0.0));
    NVISION_REQUIRE(mapped.x() == 1.0);
    NVISION_REQUIRE(mapped.y() == 1.0);
}

void crop_rejects_a_region_it_cannot_honour() {
    const ToolRegistry definitions = create_builtin_registry();
    const ToolImplementationRegistry implementations = create_vision_implementation_registry();
    const auto binding = implementations.bind(
        ToolTypeId("nvision.tool.processing.crop"), initial_version(), definitions);
    const ImageView image = sample_image();

    const auto failure_code = [&binding, &image](ParameterValue roi) {
        const ToolExecutionOutcome outcome = execute_tool(
            *binding.implementation(), crop_context(binding.definition(), image, std::move(roi)));
        NVISION_REQUIRE(!outcome.is_success());
        NVISION_REQUIRE(outcome.outputs().empty());
        return outcome.errors().front().code();
    };

    // A region authored against another coordinate space is never reinterpreted.
    NVISION_REQUIRE(
        failure_code(ParameterValue::region_of_interest(AxisAlignedRectangle(
            nvision::core::CoordinateSpaceId("nvision.frame.other.image-pixel"),
            0.0,
            0.0,
            2.0,
            2.0))) == ResultCode("COORDINATE_SPACE_MISMATCH"));

    NVISION_REQUIRE(
        failure_code(ParameterValue::region_of_interest(
            AxisAlignedRectangle(image.coordinate_space(), 3.0, 0.0, 2.0, 2.0))) ==
        ResultCode("ROI_OUT_OF_BOUNDS"));

    NVISION_REQUIRE(
        failure_code(ParameterValue::region_of_interest(
            AxisAlignedRectangle(image.coordinate_space(), 0.5, 0.0, 2.0, 2.0))) ==
        ResultCode("ROI_NOT_PIXEL_ALIGNED"));

    // The catalog allows circular regions; this implementation reports that it cannot
    // honour one instead of approximating it with a bounding box.
    NVISION_REQUIRE(
        failure_code(ParameterValue::region_of_interest(
            Circle(Point2D(image.coordinate_space(), 2.0, 1.0), 1.0))) ==
        ResultCode("UNSUPPORTED_ROI_SHAPE"));
}

}  // namespace

int main() {
    typed_port_values_enforce_the_declared_data_kind();
    parameter_binding_resolves_defaults_and_rejects_incomplete_configuration();
    tool_outcomes_never_present_a_failure_as_a_result();
    cancellation_and_budget_are_observable_through_the_execution_contract();
    the_implementation_registry_reports_unimplemented_definitions_explicitly();
    crop_executes_through_the_catalog_contract_without_copying_pixels();
    crop_rejects_a_region_it_cannot_honour();
    return 0;
}
