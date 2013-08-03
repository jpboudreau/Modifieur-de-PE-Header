#ifndef CLSECTION_H
#define CLSECTION_H

#include <windows.h>
#include <QString>
#include <QObject>

class ClSection : public QObject
{
public:
    ClSection(PIMAGE_SECTION_HEADER p_pImageSectionHeader, LPVOID);
    ~ClSection();

    const QString Nom() const;
    const int Taille() const;
    const PBYTE OpCode() const;

private:
    PIMAGE_SECTION_HEADER m_pImageSectionHeader;
    LPVOID m_pBaseFichier;
};

#endif // CLSECTION_H
