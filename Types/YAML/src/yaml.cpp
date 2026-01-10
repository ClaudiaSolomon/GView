#include "yaml.hpp"
#include <algorithm>

using namespace AppCUI;
using namespace AppCUI::Utils;
using namespace AppCUI::Application;
using namespace AppCUI::Controls;
using namespace GView::Utils;
using namespace GView::Type;
using namespace GView;
using namespace GView::View;

extern "C" {
PLUGIN_EXPORT bool Validate(const AppCUI::Utils::BufferView& buf, const std::string_view& extension)
{
    // Debug print: show extension passed to Validate
    printf("[YAML::Validate] Extension received: '%.*s'\n", static_cast<int>(extension.size()), extension.data());
    if (extension.empty())
        return false;

    auto lower_ext = std::string(extension);
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    return (lower_ext == ".yaml" || lower_ext == ".yml");
}
PLUGIN_EXPORT TypeInterface* CreateInstance()
{
    return new YAML::YAMLFile();
}
PLUGIN_EXPORT bool PopulateWindow(Reference<WindowInterface> win)
{
    auto yaml = win->GetObject()->GetContentType<YAML::YAMLFile>();

    LexicalViewer::Settings settings;
    settings.SetParser(yaml.ToObjectRef<LexicalViewer::ParseInterface>());

    win->CreateViewer(settings);

    win->CreateViewer<TextViewer::Settings>("Text View");

    GView::View::BufferViewer::Settings s{};
    yaml->selectionZoneInterface = win->GetSelectionZoneInterfaceFromViewerCreation(s);

    // add panels
    win->AddPanel(Pointer<TabPage>(new YAML::Panels::Information(yaml)), true);

    return true;
}
PLUGIN_EXPORT void UpdateSettings(IniSection sect)
{
    sect["Extension"]   = { "yaml", "yml" };
    sect["Priority"]    = 1;
    sect["Description"] = "YAML Ain't Markup Language file format (*.yaml, *.yml)";
}
}

int main()
{
    return 0;
}