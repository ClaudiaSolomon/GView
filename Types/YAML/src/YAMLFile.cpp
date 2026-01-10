#include "yaml.hpp"

namespace GView::Type::YAML
{
using namespace GView::View::LexicalViewer;

namespace CharacterType
{
    constexpr uint32 dash                = 0;
    constexpr uint32 double_quotes       = 1;
    constexpr uint32 colon               = 2;
    constexpr uint32 alphanum_characters = 3;
    constexpr uint32 spaces              = 4;
    constexpr uint32 comment             = 5;
    constexpr uint32 invalid             = 6;

    uint32 GetCharacterType(char16 ch)
    {
        if (ch == '-')
            return dash;
        if (ch == '"')
            return double_quotes;
        if (ch == ':')
            return colon;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.')
            return alphanum_characters;
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            return spaces;
        if (ch == '#')
            return comment;
        return invalid;
    }
} // namespace CharacterType

YAMLFile::YAMLFile()
{
}

void YAMLFile::ParseFile(GView::View::LexicalViewer::SyntaxManager& syntax)
{
    auto len  = syntax.text.Len();
    auto pos  = 0u;
    auto next = 0u;
    while (pos < len)
    {
        auto char_type = CharacterType::GetCharacterType(syntax.text[pos]);
        switch (char_type)
        {
        case CharacterType::dash:
            syntax.tokens.Add(TokenType::dash, pos, pos + 1, TokenColor::Operator, TokenDataType::None,
                  TokenAlignament::StartsOnNewLine | TokenAlignament::NewLineAfter, TokenFlags::DisableSimilaritySearch);
            pos++;
            break;
        case CharacterType::colon:
            syntax.tokens.Add(TokenType::colon, pos, pos + 1, TokenColor::Operator, TokenDataType::None,
                  TokenAlignament::AddSpaceAfter | TokenAlignament::SameColumn, TokenFlags::DisableSimilaritySearch);
            pos++;
            break;
        case CharacterType::comment:
            next = pos + 1;
            while (next < len && syntax.text[next] != '\n')
                next++;
            syntax.tokens.Add(TokenType::comment, pos, next, TokenColor::Comment);
            pos = next;
            break;
        case CharacterType::spaces:
            pos = syntax.text.ParseSpace(pos, SpaceType::All);
            break;
        case CharacterType::double_quotes:
            next = syntax.text.ParseString(pos, StringFormat::DoubleQuotes);
            if (syntax.tokens.GetLastTokenID() == TokenType::colon)
            {
                syntax.tokens.Add(TokenType::value, pos, next, TokenColor::Word, TokenAlignament::AddSpaceBefore);
            }
            else
            {
                syntax.tokens.Add(TokenType::key, pos, next, TokenColor::Keyword, 
                      TokenAlignament::StartsOnNewLine | TokenAlignament::AddSpaceAfter);
            }
            pos = next;
            break;
        case CharacterType::alphanum_characters:
            next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            syntax.tokens.Add(TokenType::scalar, pos, next, TokenColor::Word, TokenAlignament::AddSpaceBefore);
            pos = next;
            break;
        default:
            next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            syntax.tokens.Add(TokenType::invalid, pos, next, TokenColor::Error, TokenAlignament::AddSpaceBefore)
                  .SetError("Invalid character for YAML file");
            pos = next;
            break;
        }
    }
}
void YAMLFile::BuildBlocks(GView::View::LexicalViewer::SyntaxManager& syntax)
{
    auto len = syntax.tokens.Len();
    LocalString<128> tmp;

    for (auto index = 0u; index < len; index++)
    {
        auto token = syntax.tokens[index];
    }
}

void YAMLFile::PreprocessText(GView::View::LexicalViewer::TextEditor&)
{
    // nothing to do --> there is no pre-processing needed for a YAML format
}
void YAMLFile::GetTokenIDStringRepresentation(uint32 id, AppCUI::Utils::String& str)
{
    switch (id)
    {
    default:
        str.SetFormat("Unknown: 0x%08X", id);
        break;
    }
}
void YAMLFile::AnalyzeText(GView::View::LexicalViewer::SyntaxManager& syntax)
{
    ParseFile(syntax);
    BuildBlocks(syntax);
}
bool YAMLFile::StringToContent(std::u16string_view string, AppCUI::Utils::UnicodeStringBuilder& result)
{
    return TextParser::ExtractContentFromString(string, result, StringFormat::All);
}
bool YAMLFile::ContentToString(std::u16string_view content, AppCUI::Utils::UnicodeStringBuilder& result)
{
    NOT_IMPLEMENTED(false);
}

GView::Utils::JsonBuilderInterface* YAMLFile::GetSmartAssistantContext(const std::string_view& prompt, std::string_view displayPrompt)
{
    auto builder = GView::Utils::JsonBuilderInterface::Create();
    return builder;
}
} // namespace GView::Type::YAML