//---------------------------------------------------------------------------

#pragma hdrstop
#define GLFW_INCLUDE_GLU
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include "commons.h"
#include "commons_usr.h"
#include "include/VBO.h"
//#include "usefull.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

CMesh::CMesh() { // utworzenie pustego obiektu
  m_pVNT = NULL;
  m_nVertexCount = -1;
  m_nVBOVertices = 0; // nie zarezerwowane
};

CMesh::~CMesh() { // usuwanie obiektu
  Clear(); // zwolnienie zasobów
};

void CMesh::MakeArray(int n) { // tworzenie tablic
  m_nVertexCount = n;
  m_pVNT =
      new CVertNormTex[m_nVertexCount]; // przydzielenie pamięci dla tablicy
};

void CMesh::BuildVBOs(
    bool del) { // tworzenie VBO i kasowanie już niepotrzebnych tablic
  // pobierz numer VBO oraz ustaw go jako aktywny
  glGenBuffersARB(1, &m_nVBOVertices); // pobierz numer
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,
                  m_nVBOVertices); // ustaw bufor jako aktualny
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_nVertexCount * sizeof(CVertNormTex),
                  m_pVNT, GL_STATIC_DRAW_ARB);
  // WriteLog("Assigned VBO number "+AnsiString(m_nVBOVertices)+", vertices:
  // "+AnsiString(m_nVertexCount));
  if (del)
    SafeDeleteArray(m_pVNT); // wierzchołki już się nie przydadzą
};

void CMesh::Clear() { // niewirtualne zwolnienie zasobów przez sprzątacz albo
                      // destruktor
  // inna nazwa, żeby nie mieszało się z funkcją wirtualną sprzątacza
  if (m_nVBOVertices) // jeśli było coś rezerwowane
  {
    glDeleteBuffersARB(1, &m_nVBOVertices); // Free The Memory
    // WriteLog("Released VBO number "+AnsiString(m_nVBOVertices));
  }
  m_nVBOVertices = 0;
  m_nVertexCount = -1; // do ponownego zliczenia
  SafeDeleteArray(m_pVNT); // usuwanie tablic, gdy były użyte do Vertex Array
};

bool CMesh::StartVBO() { // początek rysowania elementów z VBO
  if (m_nVertexCount <= 0)
    return false; // nie ma nic do rysowania w ten sposób
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  if (m_nVBOVertices) {
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOVertices);
    glVertexPointer(3, GL_FLOAT, sizeof(CVertNormTex),
                    ((char *)NULL)); // pozycje
    glNormalPointer(GL_FLOAT, sizeof(CVertNormTex),
                    ((char *)NULL) + 12); // normalne
    glTexCoordPointer(2, GL_FLOAT, sizeof(CVertNormTex),
                      ((char *)NULL) + 24); // wierzchołki
  }
  return true; // można rysować z VBO
};

bool CMesh::StartColorVBO() { // początek rysowania punktów świecących z VBO
  if (m_nVertexCount <= 0)
    return false; // nie ma nic do rysowania w ten sposób
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  if (m_nVBOVertices) {
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOVertices);
    glVertexPointer(3, GL_FLOAT, sizeof(CVertNormTex),
                    ((char *)NULL)); // pozycje
    // glColorPointer(3,GL_UNSIGNED_BYTE,sizeof(CVertNormTex),((char*)NULL)+12);
    // //kolory
    glColorPointer(3, GL_FLOAT, sizeof(CVertNormTex),
                   ((char *)NULL) + 12); // kolory
  }
  return true; // można rysować z VBO
};

void CMesh::EndVBO() { // koniec użycia VBO
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  // glBindBuffer(GL_ARRAY_BUFFER,0); //takie coś psuje, mimo iż polecali użyć
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
  glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0); // Ra: to na przyszłość
};
