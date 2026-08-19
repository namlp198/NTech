using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Data.Sqlite;
using NTech.NVision.Contracts.Inspection;

namespace NTech.NVision.Engine.Inspection.Storage;

public sealed class SqliteInspectionResultRepository : IInspectionResultRepository
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        Converters = { new JsonStringEnumConverter() },
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
    };

    private readonly string connectionString;

    public SqliteInspectionResultRepository(string databasePath)
    {
        if (string.IsNullOrWhiteSpace(databasePath))
        {
            throw new ArgumentException("Inspection database path is required.", nameof(databasePath));
        }

        string path = Path.GetFullPath(databasePath);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        connectionString = new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Pooling = false,
        }.ToString();
    }

    public async ValueTask InitializeAsync(CancellationToken cancellationToken = default)
    {
        await using var connection = new SqliteConnection(connectionString);
        await connection.OpenAsync(cancellationToken).ConfigureAwait(false);
        await using SqliteCommand command = connection.CreateCommand();
        command.CommandText = """
            CREATE TABLE IF NOT EXISTS inspections (
                inspection_id TEXT PRIMARY KEY,
                station_code TEXT NOT NULL,
                sequence INTEGER NOT NULL,
                started_at TEXT NOT NULL,
                completed_at TEXT NOT NULL,
                project_id TEXT NOT NULL,
                project_version TEXT NOT NULL,
                job_id TEXT NOT NULL,
                recipe_id TEXT NOT NULL,
                recipe_version TEXT NOT NULL,
                status TEXT NOT NULL,
                decision_code TEXT NOT NULL,
                score REAL,
                total_ms REAL NOT NULL,
                payload_json TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS ix_inspections_started_at ON inspections(started_at);
            CREATE INDEX IF NOT EXISTS ix_inspections_status ON inspections(status);
            CREATE INDEX IF NOT EXISTS ix_inspections_recipe_id ON inspections(recipe_id);
            """;
        await command.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
    }

    public async ValueTask SaveAsync(InspectionResult result, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(result);
        string payload = JsonSerializer.Serialize(result, JsonOptions);
        await using var connection = new SqliteConnection(connectionString);
        await connection.OpenAsync(cancellationToken).ConfigureAwait(false);
        await using SqliteCommand command = connection.CreateCommand();
        command.CommandText = """
            INSERT INTO inspections (
                inspection_id, station_code, sequence, started_at, completed_at,
                project_id, project_version, job_id, recipe_id, recipe_version,
                status, decision_code, score, total_ms, payload_json)
            VALUES (
                $inspection_id, $station_code, $sequence, $started_at, $completed_at,
                $project_id, $project_version, $job_id, $recipe_id, $recipe_version,
                $status, $decision_code, $score, $total_ms, $payload_json);
            """;
        command.Parameters.AddWithValue("$inspection_id", result.InspectionId);
        command.Parameters.AddWithValue("$station_code", result.StationCode);
        command.Parameters.AddWithValue("$sequence", result.Sequence);
        command.Parameters.AddWithValue("$started_at", result.StartedAt.ToString("O", CultureInfo.InvariantCulture));
        command.Parameters.AddWithValue("$completed_at", result.CompletedAt.ToString("O", CultureInfo.InvariantCulture));
        command.Parameters.AddWithValue("$project_id", result.Traceability.ProjectId);
        command.Parameters.AddWithValue("$project_version", result.Traceability.ProjectVersion);
        command.Parameters.AddWithValue("$job_id", result.Traceability.JobId);
        command.Parameters.AddWithValue("$recipe_id", result.Traceability.RecipeId);
        command.Parameters.AddWithValue("$recipe_version", result.Traceability.RecipeVersion);
        command.Parameters.AddWithValue("$status", result.Status.ToString().ToUpperInvariant());
        command.Parameters.AddWithValue("$decision_code", result.DecisionCode);
        command.Parameters.AddWithValue("$score", (object?)result.Score ?? DBNull.Value);
        command.Parameters.AddWithValue("$total_ms", result.Timing.TotalMs);
        command.Parameters.AddWithValue("$payload_json", payload);
        await command.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
    }

    public async ValueTask<InspectionResult?> GetAsync(
        string inspectionId,
        CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(inspectionId))
        {
            throw new ArgumentException("Inspection identifier is required.", nameof(inspectionId));
        }

        await using var connection = new SqliteConnection(connectionString);
        await connection.OpenAsync(cancellationToken).ConfigureAwait(false);
        await using SqliteCommand command = connection.CreateCommand();
        command.CommandText = "SELECT payload_json FROM inspections WHERE inspection_id = $inspection_id;";
        command.Parameters.AddWithValue("$inspection_id", inspectionId);
        object? payload = await command.ExecuteScalarAsync(cancellationToken).ConfigureAwait(false);
        return payload is string json
            ? JsonSerializer.Deserialize<InspectionResult>(json, JsonOptions)
            : null;
    }
}
