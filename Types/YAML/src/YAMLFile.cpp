#include "yaml.hpp"

namespace GView::Type::YAML
{
using namespace GView::View::LexicalViewer;

namespace CharacterType
{
    constexpr uint32 double_quotes       = 0;
    constexpr uint32 single_quote        = 1;
    constexpr uint32 colon               = 2;
    constexpr uint32 alphanum_characters = 3;
    constexpr uint32 spaces              = 4;
    constexpr uint32 comment             = 5;
    constexpr uint32 ampersand           = 6;  // anchor
    constexpr uint32 asterisk            = 7;  // alias
    constexpr uint32 exclamation         = 8;  // tag
    constexpr uint32 pipe                = 9;  // multi-line literal
    constexpr uint32 greater_than        = 10; // multi-line folded
    constexpr uint32 open_bracket        = 11; // flow sequence start
    constexpr uint32 close_bracket       = 12; // flow sequence end
    constexpr uint32 comma               = 13; // flow sequence separator
    constexpr uint32 invalid             = 14;

    uint32 GetCharacterType(char16 ch)
    {
        if (ch == '"')
            return double_quotes;
        if (ch == '\'')
            return single_quote;
        if (ch == ':')
            return colon;
        if (ch == '&')
            return ampersand;
        if (ch == '*')
            return asterisk;
        if (ch == '!')
            return exclamation;
        if (ch == '|')
            return pipe;
        if (ch == '>')
            return greater_than;
        if (ch == '[')
            return open_bracket;
        if (ch == ']')
            return close_bracket;
        if (ch == ',')
            return comma;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '/' || ch == '-')
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

static bool IsNumericString(const LocalString<64>& text)
{
    if (text.Len() == 0)
        return false;
    
    bool hasDigit = false;
    bool hasDot = false;
    
    auto textPtr = text.GetText();
    auto textLen = text.Len();
    
    for (uint32 i = 0; i < textLen; i++)
    {
        char ch = (char)textPtr[i];
        if (ch >= '0' && ch <= '9')
        {
            hasDigit = true;
        }
        else if (ch == '.' && !hasDot)
        {
            hasDot = true;
        }
        else if (ch == '-' && i == 0)
        {
            continue;
        }
        else
        {
            return false;
        }
    }
    
    return hasDigit;
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
        case CharacterType::colon:
            syntax.tokens.Add(TokenType::colon, pos, pos + 1, TokenColor::Operator, TokenDataType::None,
                  TokenAlignament::AddSpaceAfter, TokenFlags::DisableSimilaritySearch);
            pos++;
            break;
        case CharacterType::comment:
            next = pos + 1;
            while (next < len && syntax.text[next] != '\n')
                next++;
            syntax.tokens.Add(TokenType::comment, pos, next, TokenColor::Comment, TokenAlignament::AddSpaceBefore);
            pos = next;
            break;
        case CharacterType::ampersand:
            // Anchor (&anchor_name)
            next = pos + 1;
            while (next < len && CharacterType::GetCharacterType(syntax.text[next]) == CharacterType::alphanum_characters)
                next++;
            syntax.tokens.Add(TokenType::anchor, pos, next, TokenColor::Keyword, TokenAlignament::AddSpaceBefore);
            pos = next;
            break;
        case CharacterType::asterisk:
            // Alias (*alias_name)
            next = pos + 1;
            while (next < len && CharacterType::GetCharacterType(syntax.text[next]) == CharacterType::alphanum_characters)
                next++;
            syntax.tokens.Add(TokenType::alias, pos, next, TokenColor::Keyword, TokenAlignament::AddSpaceBefore);
            pos = next;
            break;
        case CharacterType::exclamation:
            // Tag (!!str, !!int, !custom)
            next = pos + 1;
            if (next < len && syntax.text[next] == '!')
                next++;
            while (next < len && CharacterType::GetCharacterType(syntax.text[next]) == CharacterType::alphanum_characters)
                next++;
            syntax.tokens.Add(TokenType::tag, pos, next, TokenColor::Keyword, TokenAlignament::AddSpaceBefore);
            pos = next;
            break;
        case CharacterType::pipe:
        case CharacterType::greater_than:
            // Multi-line string indicator (| or >)
            syntax.tokens.Add(TokenType::scalar, pos, pos + 1, TokenColor::Operator, TokenAlignament::AddSpaceAfter);
            pos++;
            break;
        case CharacterType::spaces:
            pos = syntax.text.ParseSpace(pos, SpaceType::All);
            break;
        case CharacterType::open_bracket:
            syntax.tokens.Add(TokenType::sequence, pos, pos + 1, TokenColor::Operator, TokenAlignament::None);
            pos++;
            break;
        case CharacterType::close_bracket:
            syntax.tokens.Add(TokenType::sequence, pos, pos + 1, TokenColor::Operator, TokenAlignament::None);
            pos++;
            break;
        case CharacterType::comma:
            syntax.tokens.Add(TokenType::sequence, pos, pos + 1, TokenColor::Operator, TokenAlignament::None);
            pos++;
            break;
        case CharacterType::double_quotes:
        case CharacterType::single_quote:
            next = syntax.text.ParseString(pos, char_type == CharacterType::double_quotes ? StringFormat::DoubleQuotes : StringFormat::SingleQuotes);
            {
                auto lastToken = syntax.tokens.GetLastTokenID();
                if (lastToken == TokenType::colon)
                {
                    syntax.tokens.Add(TokenType::string, pos, next, TokenColor::String, TokenAlignament::AddSpaceBefore);
                }
                else
                {
                    auto alignment = (lastToken == TokenType::dash) ? TokenAlignament::AddSpaceBefore : TokenAlignament::StartsOnNewLine;
                    syntax.tokens.Add(TokenType::key, pos, next, TokenColor::Keyword, alignment);
                }
            }
            pos = next;
            break;
        case CharacterType::alphanum_characters:
            {
                if (syntax.text[pos] == '-')
                {
                    auto checkPos = pos;
                    while (checkPos > 0 && syntax.text[checkPos - 1] != '\n' && syntax.text[checkPos - 1] != '\r')
                        checkPos--;
                    while (checkPos < pos && (syntax.text[checkPos] == ' ' || syntax.text[checkPos] == '\t'))
                        checkPos++;
                    
                    if (checkPos == pos)
                    {
                        syntax.tokens.Add(TokenType::dash, pos, pos + 1, TokenColor::Operator, TokenDataType::None,
                              TokenAlignament::StartsOnNewLine, TokenFlags::DisableSimilaritySearch);
                        pos++;
                        break;
                    }
                }
                
                next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
                
                LocalString<64> text;
                for (auto i = pos; i < next; i++)
                    text.AddChar(syntax.text[i]);
                
                auto lastToken = syntax.tokens.GetLastTokenID();
                
                auto peek = next;
                while (peek < len && (syntax.text[peek] == ' ' || syntax.text[peek] == '\t'))
                    peek++;
                bool hasColonAfter = (peek < len && syntax.text[peek] == ':');
                
                if (hasColonAfter)
                {
                    auto alignment = (lastToken == TokenType::dash) ? TokenAlignament::AddSpaceBefore : TokenAlignament::StartsOnNewLine;
                    syntax.tokens.Add(TokenType::key, pos, next, TokenColor::Keyword, alignment);
                }
                else if (lastToken == TokenType::colon || lastToken == TokenType::dash)
                {
                    if (text == "true" || text == "false")
                    {
                        syntax.tokens.Add(TokenType::boolean, pos, next, TokenColor::Keyword, TokenAlignament::AddSpaceBefore);
                    }
                    else if (text == "null" || text == "~")
                    {
                        syntax.tokens.Add(TokenType::null_type, pos, next, TokenColor::Keyword, TokenAlignament::AddSpaceBefore);
                    }
                    else if (IsNumericString(text))
                    {
                        syntax.tokens.Add(TokenType::number, pos, next, TokenColor::Number, TokenAlignament::AddSpaceBefore);
                    }
                    else
                    {
                        syntax.tokens.Add(TokenType::value, pos, next, TokenColor::String, TokenAlignament::AddSpaceBefore);
                    }
                }
                else
                {
                    if (text == "true" || text == "false")
                    {
                        syntax.tokens.Add(TokenType::boolean, pos, next, TokenColor::Keyword, TokenAlignament::StartsOnNewLine);
                    }
                    else if (text == "null" || text == "~")
                    {
                        syntax.tokens.Add(TokenType::null_type, pos, next, TokenColor::Keyword, TokenAlignament::StartsOnNewLine);
                    }
                    else if (IsNumericString(text))
                    {
                        syntax.tokens.Add(TokenType::number, pos, next, TokenColor::Number, TokenAlignament::StartsOnNewLine);
                    }
                    else
                    {
                        syntax.tokens.Add(TokenType::value, pos, next, TokenColor::String, TokenAlignament::StartsOnNewLine);
                    }
                }
                pos = next;
            }
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
    
    struct KeyBlock {
        uint32 keyIndex;
        uint32 indent;
        uint32 endIndex;
    };
    
    std::vector<KeyBlock> blocks;
    
    for (auto index = 0u; index < len; index++)
    {
        auto token = syntax.tokens[index];
        auto tokenType = token.GetTypeID(TokenType::invalid);

        if (tokenType != TokenType::key)
            continue;
        
        auto tokenOffsetOpt = token.GetTokenStartOffset();
        if (!tokenOffsetOpt.has_value())
            continue;
        
        auto tokenOffset = tokenOffsetOpt.value();
        auto lineStart = tokenOffset;
        
        while (lineStart > 0 && syntax.text[lineStart - 1] != '\n' && syntax.text[lineStart - 1] != '\r')
            lineStart--;
        
        uint32 indent = 0;
        for (auto i = lineStart; i < tokenOffset; i++)
        {
            if (syntax.text[i] == ' ')
                indent++;
            else if (syntax.text[i] == '\t')
                indent += 4;
        }

        if (index + 1 < len && syntax.tokens[index + 1].GetTypeID(TokenType::invalid) == TokenType::colon)
        {
            bool hasInlineValue = false;
            if (index + 2 < len)
            {
                auto valueToken = syntax.tokens[index + 2];
                auto valueType = valueToken.GetTypeID(TokenType::invalid);
                if (valueType == TokenType::value || valueType == TokenType::string || 
                    valueType == TokenType::number || valueType == TokenType::boolean || 
                    valueType == TokenType::null_type)
                {
                    hasInlineValue = true;
                }
            }
            
            if (!hasInlineValue)
            {
                uint32 blockEnd = len - 1;
                
                for (auto searchIndex = index + 1; searchIndex < len; searchIndex++)
                {
                    auto searchToken = syntax.tokens[searchIndex];
                    auto searchType = searchToken.GetTypeID(TokenType::invalid);
                    
                    if (searchType == TokenType::key)
                    {
                        auto searchOffsetOpt = searchToken.GetTokenStartOffset();
                        if (!searchOffsetOpt.has_value())
                            continue;
                        
                        auto searchOffset = searchOffsetOpt.value();
                        auto searchLineStart = searchOffset;
                        
                        while (searchLineStart > 0 && syntax.text[searchLineStart - 1] != '\n' && syntax.text[searchLineStart - 1] != '\r')
                            searchLineStart--;
                        
                        uint32 searchIndent = 0;
                        for (auto i = searchLineStart; i < searchOffset; i++)
                        {
                            if (syntax.text[i] == ' ')
                                searchIndent++;
                            else if (syntax.text[i] == '\t')
                                searchIndent += 4;
                        }
                        
                        if (searchIndent <= indent)
                        {
                            blockEnd = searchIndex - 1;
                            break;
                        }
                    }
                }
                
                if (blockEnd > index)
                {
                    blocks.push_back({index, indent, blockEnd});
                }
            }
        }
    }
    
    for (const auto& blockInfo : blocks)
    {
        syntax.blocks.Add(blockInfo.keyIndex, blockInfo.endIndex, BlockAlignament::ParentBlock, BlockFlags::None);
    }
    
    auto blockCount = syntax.blocks.Len();
    for (auto index = 0u; index < blockCount; index++)
    {
        auto block = syntax.blocks[index];
        auto tokenCount = block.GetEndToken().GetIndex() - block.GetStartToken().GetIndex();
        block.SetFoldMessage(tmp.Format("Tokens: %d", tokenCount));
    }
}

void YAMLFile::PreprocessText(GView::View::LexicalViewer::TextEditor&)
{
    // nothing to do, there is no pre-processing needed for a YAML format
}
void YAMLFile::GetTokenIDStringRepresentation(uint32 id, AppCUI::Utils::String& str)
{
    switch (id)
    {
    case TokenType::key:
        str.Set("Key");
        break;
    case TokenType::value:
        str.Set("Value");
        break;
    case TokenType::string:
        str.Set("String");
        break;
    case TokenType::number:
        str.Set("Number");
        break;
    case TokenType::boolean:
        str.Set("Boolean");
        break;
    case TokenType::null_type:
        str.Set("Null");
        break;
    case TokenType::colon:
        str.Set("Colon");
        break;
    case TokenType::dash:
        str.Set("Dash");
        break;
    case TokenType::anchor:
        str.Set("Anchor");
        break;
    case TokenType::alias:
        str.Set("Alias");
        break;
    case TokenType::tag:
        str.Set("Tag");
        break;
    case TokenType::comment:
        str.Set("Comment");
        break;
    case TokenType::scalar:
        str.Set("Scalar");
        break;
    case TokenType::mapping:
        str.Set("Mapping");
        break;
    case TokenType::sequence:
        str.Set("Sequence");
        break;
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