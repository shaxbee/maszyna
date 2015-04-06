//---------------------------------------------------------------------------

#ifndef NamesH
#define NamesH
//---------------------------------------------------------------------------
class ItemRecord
{ // rekord opisujący obiekt; raz utworzony nie przemieszcza się
    // rozmiar rekordu można zmienić w razie potrzeby
  public:
    char *cName; // wskaźnik do nazwy umieszczonej w buforze
    int iFlags; // flagi bitowe
    ItemRecord *rPrev, *rNext; // posortowane drzewo (przebudowywane w razie potrzeby)
    union
    {
        void *pData; // wskaźnik do obiektu
        int iData; // albo numer obiektu (tekstury)
        unsigned int uData;
    };
    // typedef
    void ListGet(ItemRecord *r, int *&n);
    void TreeAdd(ItemRecord *r, int c);
    template <typename TOut> inline TOut *DataGet() { return (TOut *)pData; };
    template <typename TOut> inline void DataSet(TOut *x) { pData = (void *)x; };
    void *__fastcall TreeFind(const char *n);
    ItemRecord *__fastcall TreeFindRecord(const char *n);
};

class TNames
{
  public:
    int iSize; // rozmiar bufora
    char *cBuffer; // bufor dla rekordów (na początku) i nazw (na końcu)
    ItemRecord *rRecords; // rekordy na początku bufora
    char *cLast; // ostatni użyty bajt na nazwy
    ItemRecord *rTypes[20]; // rożne typy obiektów (początek drzewa)
    int iLast; // ostatnio użyty rekord
  public:
    TNames();
    int Add(int t, const char *n); // dodanie obiektu typu (t)
    int Add(int t, const char *n, void *d); // dodanie obiektu z wskaźnikiem
    int Add(int t, const char *n, int d); // dodanie obiektu z numerem
    bool Update(int t, const char *n,
                           void *d); // dodanie jeśli nie ma, wymiana (d), gdy jest
    void TreeSet();
    ItemRecord *__fastcall TreeSet(int *n, int d, int u);
    void Sort(int t); // przebudowa drzewa typu (t)
    ItemRecord *__fastcall Item(int n); // rekord o numerze (n)
    inline void *Find(const int t, const char *n)
    {
        return rTypes[t] ? rTypes[t]->TreeFind(n) : NULL;
    };
    ItemRecord *__fastcall FindRecord(const int t, const char *n);
    // template <typename TOut> inline TOut* Find(const int t,const char *n)
    //{return (TOut*)(rTypes[t]->TreeFind(n));};
};
#endif
