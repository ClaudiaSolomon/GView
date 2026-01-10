#include "yaml.hpp"

using namespace GView::Type::YAML;
using namespace AppCUI::Controls;

Panels::Information::Information(Reference<GView::Type::YAML::YAMLFile> _yaml) : TabPage("&Information")
{
    yaml    = _yaml;
    general = Factory::ListView::Create(this, "x:0,y:0,w:100%,h:10", { "n:Field,w:12", "n:Value,w:100" }, ListViewFlags::None);

    issues = Factory::ListView::Create(this, "x:0,y:21,w:100%,h:10", { "n:Info,w:200" }, ListViewFlags::HideColumns);

    this->Update();
}
void Panels::Information::UpdateGeneralInformation()
{
    LocalString<256> tempStr;
    NumericFormatter n;

    general->DeleteAllItems();
    general->AddItem("File");
}

void Panels::Information::UpdateIssues()
{
}
void Panels::Information::RecomputePanelsPositions()
{
    int py   = 0;
    int last = 0;
    int w    = this->GetWidth();
    int h    = this->GetHeight();

    if ((!general.IsValid()) || (!issues.IsValid()))
        return;

    issues->SetVisible(false);
    this->general->Resize(w, h);
}
void Panels::Information::Update()
{
    UpdateGeneralInformation();
    UpdateIssues();
    RecomputePanelsPositions();
}
