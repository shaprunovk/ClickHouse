#pragma once

#include <Core/Block.h>
#include <Processors/Formats/IOutputFormat.h>
#include <Formats/FormatSettings.h>
#include <Formats/FormatFactory.h>


namespace DB
{

class WriteBuffer;
class Context;


/** Prints the result in the form of beautiful tables.
  */
class PrettyBlockOutputFormat : public IOutputFormat
{
public:
    /// no_escapes - do not use ANSI escape sequences - to display in the browser, not in the console.
    PrettyBlockOutputFormat(WriteBuffer & out_, const Block & header_, const FormatSettings & format_settings_, bool mono_block_);

    String getName() const override { return "PrettyBlockOutputFormat"; }

protected:
    void consume(Chunk) override;
    void consumeTotals(Chunk) override;
    void consumeExtremes(Chunk) override;

    size_t total_rows = 0;
    size_t row_number_width = 7; // "10000. "

    const FormatSettings format_settings;
    Serializations serializations;

    using Widths = PODArray<size_t>;
    using WidthsPerColumn = std::vector<Widths>;

    void write(Chunk chunk, PortKind port_kind);
    virtual void writeChunk(const Chunk & chunk, PortKind port_kind);
    void writeSquashedChunkIfNeeded();
    void writeSuffix() override;

    void onRowsReadBeforeUpdate() override { total_rows = getRowsReadBefore(); }

    void calculateWidths(
        const Block & header, const Chunk & chunk,
        WidthsPerColumn & widths, Widths & max_padded_widths, Widths & name_widths);

    void writeValueWithPadding(
        const IColumn & column, const ISerialization & serialization, size_t row_num,
        size_t value_width, size_t pad_to_width, bool align_right);

    void resetFormatterImpl() override
    {
        total_rows = 0;
    }

private:
    bool mono_block;
    Chunk squashed_chunk;
    Stopwatch time_after_previous_chunk; /// For squashing.
};

template <typename OutputFormat>
void registerPrettyFormatWithNoEscapesAndMonoBlock(FormatFactory & factory, const String & base_name)
{
    auto creator = [&](FormatFactory & fact, const String & name, bool no_escapes, bool mono_block)
    {
        fact.registerOutputFormat(name, [no_escapes, mono_block](
            WriteBuffer & buf,
            const Block & sample,
            const FormatSettings & format_settings)
        {
            if (no_escapes)
            {
                FormatSettings changed_settings = format_settings;
                changed_settings.pretty.color = false;
                return std::make_shared<OutputFormat>(buf, sample, changed_settings, mono_block);
            }
            return std::make_shared<OutputFormat>(buf, sample, format_settings, mono_block);
        });
    };
    creator(factory, base_name, false, false);
    creator(factory, base_name + "NoEscapes", true, false);
    creator(factory, base_name + "MonoBlock", false, true);
    creator(factory, base_name + "NoEscapesMonoBlock", true, true);
}

}
