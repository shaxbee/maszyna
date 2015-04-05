#include "Button.h"
#include "Console.h"

TButton::TButton()
{
    iFeedbackBit = 0;
    Clear();
};

TButton::~TButton(){};

void TButton::Clear(int i)
{
    pModelOn = NULL;
    pModelOff = NULL;
    bOn = false;
    if (i >= 0)
        FeedbackBitSet(i);
    Update(); // kasowanie bitu Feedback, o ile jakiś ustawiony
};

void TButton::Init(const std::string name, TModel3d *pModel, bool bNewOn)
{
    if (!pModel)
        return; // nie ma w czym szukać
    pModelOn = pModel->GetFromName(name + "_on");
    pModelOff = pModel->GetFromName(name + "_off");
    bOn = bNewOn;
    Update();
};

void TButton::Load(TQueryParserComp *Parser, TModel3d *pModel1, TModel3d *pModel2)
{
    AnsiString str = Parser->GetNextSymbol().LowerCase();
    if (pModel1)
    { // poszukiwanie submodeli w modelu
        Init(str, pModel1, false);
        if (pModel2)
            if (!pModelOn && !pModelOff)
                Init(str, pModel2,
                     false); // może w drugim będzie (jak nie w kabinie, to w zewnętrznym)
    }
    else
    {
        pModelOn = NULL;
        pModelOff = NULL;
    }
};

void TButton::Update()
{
    if (pModelOn)
        pModelOn->iVisible = bOn;
    if (pModelOff)
        pModelOff->iVisible = !bOn;
    if (iFeedbackBit) // jeżeli generuje informację zwrotną
    {
        if (bOn) // zapalenie
            Console::BitsSet(iFeedbackBit);
        else
            Console::BitsClear(iFeedbackBit);
    }
};
