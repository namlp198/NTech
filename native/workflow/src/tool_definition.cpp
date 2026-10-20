#include "nvision/workflow/tool_definition.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nvision::workflow {

namespace {

constexpr std::size_t maximum_display_name_length = 128U;
constexpr std::size_t maximum_category_segment_length = 64U;
constexpr std::size_t maximum_tag_length = 64U;
constexpr std::size_t maximum_description_length = 4'096U;

bool is_valid(const PortDirection direction) noexcept {
    return direction == PortDirection::input || direction == PortDirection::output;
}

bool is_valid(const ToolOriginKind kind) noexcept {
    return kind == ToolOriginKind::built_in || kind == ToolOriginKind::plugin;
}

bool is_valid(const ToolExecutionLifetime lifetime) noexcept {
    return lifetime == ToolExecutionLifetime::stateless ||
           lifetime == ToolExecutionLifetime::per_execution ||
           lifetime == ToolExecutionLifetime::per_session ||
           lifetime == ToolExecutionLifetime::singleton_shared;
}

bool is_valid(const ToolDeviceRequirement requirement) noexcept {
    return requirement == ToolDeviceRequirement::cpu ||
           requirement == ToolDeviceRequirement::gpu_optional ||
           requirement == ToolDeviceRequirement::gpu_required ||
           requirement == ToolDeviceRequirement::external_device;
}

bool is_valid(const core::PortDataType type) noexcept {
    switch (type) {
        case core::PortDataType::frame:
        case core::PortDataType::image:
        case core::PortDataType::image_view:
        case core::PortDataType::mask:
        case core::PortDataType::region:
        case core::PortDataType::point_2d:
        case core::PortDataType::point_list:
        case core::PortDataType::line_2d:
        case core::PortDataType::circle:
        case core::PortDataType::contour:
        case core::PortDataType::contour_list:
        case core::PortDataType::bounding_box:
        case core::PortDataType::polygon:
        case core::PortDataType::transform:
        case core::PortDataType::homography:
        case core::PortDataType::feature_set:
        case core::PortDataType::match_set:
        case core::PortDataType::detection:
        case core::PortDataType::detection_list:
        case core::PortDataType::text_result:
        case core::PortDataType::barcode_result:
        case core::PortDataType::measurement:
        case core::PortDataType::measurement_list:
        case core::PortDataType::defect:
        case core::PortDataType::defect_list:
        case core::PortDataType::point_grid_model:
        case core::PortDataType::point_grid_result:
        case core::PortDataType::boolean:
        case core::PortDataType::integer:
        case core::PortDataType::floating_point:
        case core::PortDataType::string:
        case core::PortDataType::decision:
        case core::PortDataType::evidence:
        case core::PortDataType::overlay:
        case core::PortDataType::any_metadata:
            return true;
    }
    return false;
}

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](const unsigned char item) {
        return std::isspace(item) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char item) {
        return std::isspace(item) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

void validate_text(
    const std::string_view value,
    const std::size_t maximum_length,
    const std::string_view field_name) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field_name) + " must not be blank");
    }
    if (value.size() > maximum_length) {
        throw std::invalid_argument(std::string(field_name) + " exceeds its maximum length");
    }
    if (std::any_of(value.begin(), value.end(), [](const unsigned char item) {
            return item < 0x20U && item != '\t';
        })) {
        throw std::invalid_argument(std::string(field_name) + " contains a control character");
    }
}

bool supports_visibility_condition(const ParameterKind kind) noexcept {
    return kind == ParameterKind::boolean || kind == ParameterKind::integer ||
           kind == ParameterKind::string || kind == ParameterKind::enumeration;
}

}  // namespace

ToolPortDefinition::ToolPortDefinition(
    core::PortKey key,
    std::string display_name,
    const PortDirection direction,
    const core::PortDataType data_type,
    const bool required,
    const bool allows_multiple)
    : key_(std::move(key)),
      display_name_(trim(std::move(display_name))),
      direction_(direction),
      data_type_(data_type),
      required_(required),
      allows_multiple_(allows_multiple) {
    validate_text(display_name_, maximum_display_name_length, "port display name");
    if (!is_valid(direction_)) {
        throw std::invalid_argument("port direction is unsupported");
    }
    if (!is_valid(data_type_)) {
        throw std::invalid_argument("port data type is unsupported");
    }
}

const core::PortKey& ToolPortDefinition::key() const noexcept {
    return key_;
}

const std::string& ToolPortDefinition::display_name() const noexcept {
    return display_name_;
}

PortDirection ToolPortDefinition::direction() const noexcept {
    return direction_;
}

core::PortDataType ToolPortDefinition::data_type() const noexcept {
    return data_type_;
}

bool ToolPortDefinition::required() const noexcept {
    return required_;
}

bool ToolPortDefinition::allows_multiple() const noexcept {
    return allows_multiple_;
}

ToolCatalogMetadata::ToolCatalogMetadata(
    std::string display_name,
    std::vector<std::string> category_path,
    std::vector<std::string> tags,
    std::string description)
    : display_name_(trim(std::move(display_name))),
      category_path_(std::move(category_path)),
      tags_(std::move(tags)),
      description_(trim(std::move(description))) {
    validate_text(display_name_, maximum_display_name_length, "tool display name");
    if (category_path_.empty()) {
        throw std::invalid_argument("tool category path must not be empty");
    }
    for (auto& segment : category_path_) {
        segment = trim(std::move(segment));
        validate_text(segment, maximum_category_segment_length, "tool category segment");
        if (segment.find('/') != std::string::npos) {
            throw std::invalid_argument("tool category segments must not contain '/'");
        }
    }
    for (auto& tag : tags_) {
        tag = trim(std::move(tag));
        validate_text(tag, maximum_tag_length, "tool tag");
    }
    std::vector<std::string> unique_tags = tags_;
    std::sort(unique_tags.begin(), unique_tags.end());
    if (std::adjacent_find(unique_tags.begin(), unique_tags.end()) != unique_tags.end()) {
        throw std::invalid_argument("tool tags must be unique");
    }
    if (description_.size() > maximum_description_length) {
        throw std::invalid_argument("tool description exceeds its maximum length");
    }
}

const std::string& ToolCatalogMetadata::display_name() const noexcept {
    return display_name_;
}

const std::vector<std::string>& ToolCatalogMetadata::category_path() const noexcept {
    return category_path_;
}

const std::vector<std::string>& ToolCatalogMetadata::tags() const noexcept {
    return tags_;
}

const std::string& ToolCatalogMetadata::description() const noexcept {
    return description_;
}

ToolOrigin::ToolOrigin(const ToolOriginKind kind, std::optional<core::PluginId> plugin_id)
    : kind_(kind), plugin_id_(std::move(plugin_id)) {
    if (!is_valid(kind_)) {
        throw std::invalid_argument("tool origin kind is unsupported");
    }
    if ((kind_ == ToolOriginKind::plugin) != plugin_id_.has_value()) {
        throw std::invalid_argument("plugin tool origin must contain exactly one plugin ID");
    }
}

ToolOrigin ToolOrigin::built_in() {
    return ToolOrigin(ToolOriginKind::built_in, std::nullopt);
}

ToolOrigin ToolOrigin::plugin(core::PluginId plugin_id) {
    return ToolOrigin(ToolOriginKind::plugin, std::move(plugin_id));
}

ToolOriginKind ToolOrigin::kind() const noexcept {
    return kind_;
}

const std::optional<core::PluginId>& ToolOrigin::plugin_id() const noexcept {
    return plugin_id_;
}

ToolExecutionCharacteristics::ToolExecutionCharacteristics(
    const ToolExecutionLifetime lifetime,
    const ToolDeviceRequirement device_requirement,
    const bool thread_safe,
    const bool reentrant,
    const bool deterministic,
    const bool supports_cancellation,
    const bool supports_timeout,
    const bool supports_debug_preview)
    : lifetime_(lifetime),
      device_requirement_(device_requirement),
      thread_safe_(thread_safe),
      reentrant_(reentrant),
      deterministic_(deterministic),
      supports_cancellation_(supports_cancellation),
      supports_timeout_(supports_timeout),
      supports_debug_preview_(supports_debug_preview) {
    if (!is_valid(lifetime_)) {
        throw std::invalid_argument("tool execution lifetime is unsupported");
    }
    if (!is_valid(device_requirement_)) {
        throw std::invalid_argument("tool device requirement is unsupported");
    }
}

ToolExecutionLifetime ToolExecutionCharacteristics::lifetime() const noexcept {
    return lifetime_;
}

ToolDeviceRequirement ToolExecutionCharacteristics::device_requirement() const noexcept {
    return device_requirement_;
}

bool ToolExecutionCharacteristics::thread_safe() const noexcept {
    return thread_safe_;
}

bool ToolExecutionCharacteristics::reentrant() const noexcept {
    return reentrant_;
}

bool ToolExecutionCharacteristics::deterministic() const noexcept {
    return deterministic_;
}

bool ToolExecutionCharacteristics::supports_cancellation() const noexcept {
    return supports_cancellation_;
}

bool ToolExecutionCharacteristics::supports_timeout() const noexcept {
    return supports_timeout_;
}

bool ToolExecutionCharacteristics::supports_debug_preview() const noexcept {
    return supports_debug_preview_;
}

ToolTypeDefinition::ToolTypeDefinition(
    core::ToolTypeId id,
    core::SemanticVersion version,
    ToolCatalogMetadata catalog,
    std::vector<ToolPortDefinition> ports,
    ToolOrigin origin,
    ToolExecutionCharacteristics execution,
    std::vector<ParameterDefinition> parameters)
    : id_(std::move(id)),
      version_(std::move(version)),
      catalog_(std::move(catalog)),
      ports_(std::move(ports)),
      origin_(std::move(origin)),
      execution_(std::move(execution)),
      parameters_(std::move(parameters)) {
    for (std::size_t index = 0U; index < ports_.size(); ++index) {
        const auto duplicate = std::find_if(
            ports_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            ports_.end(),
            [this, index](const ToolPortDefinition& candidate) {
                return candidate.key() == ports_[index].key() &&
                       candidate.direction() == ports_[index].direction();
            });
        if (duplicate != ports_.end()) {
            throw std::invalid_argument("tool port keys must be unique within each direction");
        }
    }

    for (std::size_t index = 0U; index < parameters_.size(); ++index) {
        const auto duplicate = std::find_if(
            parameters_.begin() + static_cast<std::ptrdiff_t>(index + 1U),
            parameters_.end(),
            [this, index](const ParameterDefinition& candidate) {
                return candidate.key() == parameters_[index].key();
            });
        if (duplicate != parameters_.end()) {
            throw std::invalid_argument("tool parameter keys must be unique");
        }
    }

    for (const auto& parameter : parameters_) {
        if (!parameter.visibility()) {
            continue;
        }
        const auto* controller = find_parameter(parameter.visibility()->controller());
        if (controller == nullptr) {
            throw std::invalid_argument("visibility condition controller does not exist");
        }
        if (controller->key() == parameter.key()) {
            throw std::invalid_argument("parameter visibility must not depend on itself");
        }
        if (!supports_visibility_condition(controller->type().kind())) {
            throw std::invalid_argument("visibility controller must use a comparable scalar type");
        }
        if (!controller->type().accepts(parameter.visibility()->expected_value())) {
            throw std::invalid_argument(
                "visibility condition value does not satisfy its controller schema");
        }
    }

    for (const auto& parameter : parameters_) {
        std::vector<core::ParameterKey> visited;
        const ParameterDefinition* current = &parameter;
        while (current->visibility()) {
            if (std::find(visited.begin(), visited.end(), current->key()) != visited.end()) {
                throw std::invalid_argument("parameter visibility conditions must not form a cycle");
            }
            visited.push_back(current->key());
            current = find_parameter(current->visibility()->controller());
        }
    }
}

const core::ToolTypeId& ToolTypeDefinition::id() const noexcept {
    return id_;
}

const core::SemanticVersion& ToolTypeDefinition::version() const noexcept {
    return version_;
}

const ToolCatalogMetadata& ToolTypeDefinition::catalog() const noexcept {
    return catalog_;
}

const std::vector<ToolPortDefinition>& ToolTypeDefinition::ports() const noexcept {
    return ports_;
}

const ToolOrigin& ToolTypeDefinition::origin() const noexcept {
    return origin_;
}

const ToolExecutionCharacteristics& ToolTypeDefinition::execution() const noexcept {
    return execution_;
}

const std::vector<ParameterDefinition>& ToolTypeDefinition::parameters() const noexcept {
    return parameters_;
}

const ToolPortDefinition* ToolTypeDefinition::find_port(
    const core::PortKey& key,
    const PortDirection direction) const noexcept {
    const auto match = std::find_if(
        ports_.begin(), ports_.end(), [&key, direction](const ToolPortDefinition& port) {
            return port.key() == key && port.direction() == direction;
        });
    return match == ports_.end() ? nullptr : &*match;
}

const ParameterDefinition* ToolTypeDefinition::find_parameter(
    const core::ParameterKey& key) const noexcept {
    const auto match = std::find_if(
        parameters_.begin(), parameters_.end(), [&key](const ParameterDefinition& parameter) {
            return parameter.key() == key;
        });
    return match == parameters_.end() ? nullptr : &*match;
}

}  // namespace nvision::workflow
