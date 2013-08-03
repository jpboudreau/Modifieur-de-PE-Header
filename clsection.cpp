#include "clsection.h"

ClSection::ClSection(PIMAGE_SECTION_HEADER p_pImageSectionHeader, LPVOID p_pBasefichier)
    : m_pImageSectionHeader(p_pImageSectionHeader), m_pBaseFichier(p_pBasefichier)
{

}

ClSection::~ClSection()
{

}

const QString ClSection::Nom() const
{
    return QString((char*)m_pImageSectionHeader->Name);
}

const int ClSection::Taille() const
{
    return m_pImageSectionHeader->SizeOfRawData;
}


const PBYTE ClSection::OpCode() const
{
    return reinterpret_cast<PBYTE>((PBYTE)m_pBaseFichier + m_pImageSectionHeader->PointerToRawData);
}
