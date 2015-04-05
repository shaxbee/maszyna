#include "AirCoupler.h"
#include "Timer.h"

TAirCoupler::TAirCoupler() { Clear(); }

TAirCoupler::~TAirCoupler() {}

int TAirCoupler::GetStatus()
{ // zwraca 1, jeśli istnieje model prosty, 2 gdy skośny
    int x = 0;
    if (pModelOn)
        x = 1;
    if (pModelxOn)
        x = 2;
    return x;
}

void TAirCoupler::Clear()
{ // zerowanie wskaźników
    pModelOn = NULL;
    pModelOff = NULL;
    pModelxOn = NULL;
    bOn = false;
    bxOn = false;
}

void TAirCoupler::Init(const std::string name, TModel3d *pModel)
{ // wyszukanie submodeli
    if (!pModel)
        return;                                     // nie ma w czym szukać
    pModelOn = pModel->GetFromName(name + "_on");   // połączony na wprost
    pModelOff = pModel->GetFromName(name + "_off"); // odwieszony
    pModelxOn = pModel->GetFromName(name + "_xon"); // połączony na skos
}

void TAirCoupler::Load(cParser &parser, TModel3d *pModel)
{
    auto str = parser.readString();
    if (pModel)
    {
        Init(str, pModel);
    }
    else
    {
        pModelOn = NULL;
        pModelxOn = NULL;
        pModelOff = NULL;
    }
}

void TAirCoupler::Update()
{
    //  if ((pModelOn!=NULL) && (pModelOn!=NULL))
    {
        if (pModelOn)
            pModelOn->iVisible = bOn;
        if (pModelOff)
            pModelOff->iVisible = !(bOn || bxOn);
        if (pModelxOn)
            pModelxOn->iVisible = bxOn;
    }
}