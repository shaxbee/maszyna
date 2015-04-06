//---------------------------------------------------------------------------

#ifndef VBOH
#define VBOH
//---------------------------------------------------------------------------
class CVertNormTex
{
  public:
    float x; // X wierzchołka
    float y; // Y wierzchołka
    float z; // Z wierzchołka
    float nx; // X wektora normalnego
    float ny; // Y wektora normalnego
    float nz; // Z wektora normalnego
    float u; // U mapowania
    float v; // V mapowania
};

class CMesh
{ // wsparcie dla VBO
  public:
    int m_nVertexCount; // liczba wierzchołków
    CVertNormTex *m_pVNT;
    unsigned int m_nVBOVertices; // numer VBO z wierzchołkami
    CMesh();
    ~CMesh();
    void MakeArray(int n); // tworzenie tablicy z elementami VNT
    void BuildVBOs(bool del = true); // zamiana tablic na VBO
    void Clear(); // zwolnienie zasobów
    bool StartVBO();
    void EndVBO();
    bool StartColorVBO();
};

#endif
