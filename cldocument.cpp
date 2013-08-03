#include "cldocument.h"


namespace
{
const std::string ressourceName[] = {
    "RT_CURSOR",
    "RT_BITMAP",
    "RT_ICON",
    "RT_MENU",
    "RT_DIALOG",
    "RT_STRING",
    "RT_FONTDIR",
    "RT_FONT",
    "RT_ACCELERATOR",
    "RT_RCDATA",
    "RT_MESSAGETABLE",
    "RT_GROUP_CURSOR",
    "Vide", // Il n'y a rien à cette emplacement
    "RT_GROUP_ICON",
    "Vide", // Il n'y a rien à cette emplacement
    "RT_VERSION",
    "RT_DLGINCLUDE",
    "Vide", // Il n'y a rien à cette emplacement
    "RT_PLUGPLAY",
    "RT_VXD",
    "RT_ANICURSOR",
    "RT_ANIICON",
    "RT_HTML",
    "RT_MANIFEST",
    };

std::string NumeroRessourceEnNom(DWORD p_id)
{
    // Ajouter une vérification
    return ressourceName[p_id - 1]; // l'array commence à 0 et non à 1 comme les id de ressource
}

// Les différents DataDirectory
const QString DATA_DIRECTORY[] = {
    "Export",
    "Import",
    "Ressource",
    "Exception",
    "Certificat",
    "Relocation",
    "Debug",
    "Architecture",
    "Pointeur global",
    "TLS (Thread Local Storage)",
    "Load config",
    "Bound import",
    "Import address table",
    "Delay import",
    "COM descriptor",
    "Reservé",
    "Reservé"};

/* CodeMachineEnTexte : Retourne en texte le type d'architecture fournit
 *                      par le code machine en paramètre.
 *
 */
std::string CodeMachineEnTexte(WORD p_codeMachine)
{
    std::string texte = "";

    switch (p_codeMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
        texte = "Intel 386";
        break;
    case IMAGE_FILE_MACHINE_R3000:
        texte = "MIPS Little endian";
    case IMAGE_FILE_MACHINE_UNKNOWN:
    default:
        texte = "Inconnu";
        break;
    }

    return texte;
}

ULONGLONG VirtualAddressToFileAddress(const PIMAGE_NT_HEADERS p_ntHeader, unsigned long p_adresse)
{
    PIMAGE_FILE_HEADER fileHeader = reinterpret_cast<PIMAGE_FILE_HEADER>(&p_ntHeader->FileHeader);
    int nbSection = fileHeader->NumberOfSections;


    PIMAGE_SECTION_HEADER sectionHeader;
    PIMAGE_SECTION_HEADER sectionHeaderSuivant;

    bool offsetEstTrouve;

    // Optimisation
   /* int i = 0;
    while (i++ < nbSection && reinterpret_cast<PIMAGE_SECTION_HEADER>((ULONGLONG)p_ntHeader + sizeof(IMAGE_NT_HEADERS)
                                                   + i *sizeof(IMAGE_SECTION_HEADER))->VirtualAddress > p_adresse &&
           reinterpret_cast<PIMAGE_SECTION_HEADER>((ULONGLONG)p_ntHeader + sizeof(IMAGE_NT_HEADERS)
                                                   + i *sizeof(IMAGE_SECTION_HEADER))->VirtualAddress <= p_adresse)
    {
    }*/

    for (int i = 0; i < nbSection; ++i)
    {
        sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(reinterpret_cast<ULONGLONG>(p_ntHeader) + sizeof(IMAGE_NT_HEADERS)
                                                                + i *sizeof(IMAGE_SECTION_HEADER));
        sectionHeaderSuivant = reinterpret_cast<PIMAGE_SECTION_HEADER>(reinterpret_cast<ULONGLONG>(sectionHeader) + sizeof(IMAGE_SECTION_HEADER));

        if (sectionHeaderSuivant->VirtualAddress > p_adresse && sectionHeader->VirtualAddress <= p_adresse)
        {
            offsetEstTrouve = true;
            break;
        }
    }

    unsigned long adresseDansFichier;

    if (!offsetEstTrouve)
        sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>((ULONGLONG)p_ntHeader + sizeof(IMAGE_NT_HEADERS)
                                                                + (nbSection -1) *sizeof(IMAGE_SECTION_HEADER));

    adresseDansFichier = p_adresse - sectionHeader->VirtualAddress + sectionHeader->PointerToRawData;

    return adresseDansFichier;
}
}

ClDocument::~ClDocument()
{
    FermerFichier();
}

void ClDocument::FermerFichier()
{
    if (m_pBaseFichier != 0)
        UnmapViewOfFile(m_pBaseFichier);

    if (m_hFichierMapper != 0)
        CloseHandle(m_hFichierMapper);

    if (m_hFichier != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFichier);

    m_fichierEstOuvert = false;
}

bool ClDocument::OuvrirFichier(const QString& p_chemin)
{
    if (m_fichierEstOuvert)
        FermerFichier();

    m_hFichier = CreateFile(p_chemin.toStdWString().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (m_hFichier == INVALID_HANDLE_VALUE)
    {
        QMessageBox::information(nullptr,"TEst", "CreateFile");
        return false;
    }

    m_hFichierMapper = CreateFileMapping(m_hFichier, NULL, PAGE_READWRITE, 0, 0, NULL);

    if (m_hFichierMapper == 0)
    {
        QMessageBox::information(nullptr,"TEst", "CreateFileMapping");
        return false;
    }

    m_pBaseFichier = MapViewOfFile(m_hFichierMapper, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (m_pBaseFichier == 0)
    {
        QMessageBox::information(nullptr,"TEst", "Mapviewoffile");
        return false;
    }

    m_pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(m_pBaseFichier);

    if (m_pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return false;


    m_pNtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>((PBYTE)m_pBaseFichier + m_pDosHeader->e_lfanew);

    if (m_pNtHeader->Signature != IMAGE_NT_SIGNATURE)
        return false;

    m_pImageFileHeader = reinterpret_cast<PIMAGE_FILE_HEADER>((PBYTE)&m_pNtHeader->FileHeader);

    m_pImageOptionalHeader = reinterpret_cast<PIMAGE_OPTIONAL_HEADER>((PBYTE)&m_pNtHeader->OptionalHeader);

    // Sections
    PIMAGE_SECTION_HEADER pImageSectionHeader;
    for (int i = 0; i != m_pImageFileHeader->NumberOfSections; ++i)
    {
        pImageSectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>((PBYTE)m_pNtHeader + sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*i);

        m_sections.push_back(new ClSection(pImageSectionHeader, m_pBaseFichier));
    }

    return m_fichierEstOuvert = true;
}

QStandardItemModel* ClDocument::ObtenirModeleArbre()
{
    QStandardItemModel* modele = new QStandardItemModel;
    if (m_fichierEstOuvert)
    {
        modele->appendRow(ObtenirModeleDosHeader());
        modele->appendRow(ObtenirModeleNtHeader());
        modele->appendRow(ObtenirModeleSectionHeader());
        modele->appendRow(ObtenirModeleDataDirectory());

        QList<QStandardItem *> header;
        modele->appendColumn(header);

        modele->setHeaderData(0, Qt::Horizontal, "Portable Executable");
        modele->setHeaderData(1, Qt::Horizontal, "Description");
    }

    return modele;
}

QStandardItem* ClDocument::ObtenirModeleDosHeader()
{
    QStandardItem *dosHeader = new QStandardItem("Dos Header");
    dosHeader->appendRow(new QStandardItem("Magic"));

    std::ostringstream oss;
    oss << std::hex << "0x" << m_pDosHeader->e_magic << " (" << static_cast<char>(m_pDosHeader->e_magic & 255)
        << static_cast<char>(m_pDosHeader->e_magic >> 8) << ')';

    QList<QStandardItem *> descriptionDosHeader;
    QStandardItem* test = new QStandardItem(oss.str().c_str());
    test->setData(QVariant::fromValue(new QSharedPointer<ClType>(new ClType(&m_pDosHeader->e_magic, typeid(m_pDosHeader->e_magic)))), ClDocument::Type);

    descriptionDosHeader.append(test);
    dosHeader->appendColumn(descriptionDosHeader);

    return dosHeader;
}

QStandardItem* ClDocument::ObtenirModeleNtHeader()
{
    // Nt Header
    QStandardItem *ntHeader = new QStandardItem("NT Header");
    ntHeader->appendRow(new QStandardItem("Magic"));

    std::ostringstream oss;
    oss.str("");
    oss << std::hex << "0x" << m_pNtHeader->Signature << " (" << static_cast<char>(m_pNtHeader->Signature & 255)
        << static_cast<char>(m_pNtHeader->Signature >> 8) << ')';

    QList<QStandardItem *> descriptionNtHeader;

    QStandardItem* test = new QStandardItem(oss.str().c_str());
    test->setData(QVariant::fromValue(new QSharedPointer<ClType>(new ClType(&m_pNtHeader->Signature, typeid(m_pNtHeader->Signature)))), ClDocument::Type);

    descriptionNtHeader.append(test);
    ntHeader->appendColumn(descriptionNtHeader);

    ntHeader->appendRow(ObtenirModeleFileHeader());

    return ntHeader;
}

QStandardItem* ClDocument::ObtenirModeleFileHeader()
{
     // FileHeader
    QStandardItem *fileHeader = new QStandardItem("File Header");
    fileHeader->appendRow(new QStandardItem("Architecture"));

    std::ostringstream oss;
    oss.str("");
    oss << std::hex << "0x" << m_pImageFileHeader->Machine << " (" << CodeMachineEnTexte(m_pImageFileHeader->Machine) <<  ')';

    QList<QStandardItem *> descriptionFileHeader;
    descriptionFileHeader.append(new QStandardItem(oss.str().c_str()));

    fileHeader->appendRow(new QStandardItem("Nombre de section"));
    oss.str("");
    oss << std::dec << m_pImageFileHeader->NumberOfSections;
    descriptionFileHeader.append(new QStandardItem(oss.str().c_str()));

    fileHeader->appendRow(new QStandardItem("Compilé le"));
    oss.str("");
    struct tm tm;
    gmtime_s(&tm, (time_t *)&m_pImageFileHeader->TimeDateStamp);
    oss << std::dec << tm.tm_mday << '/' << tm.tm_mon+1 << '/' << tm.tm_year + 1900;


    QStandardItem* test = new QStandardItem(oss.str().c_str());
    test->setData(QVariant::fromValue(new QSharedPointer<ClType>(new ClType(&m_pImageFileHeader->TimeDateStamp, typeid(m_pImageFileHeader->TimeDateStamp)))), ClDocument::Type);

    descriptionFileHeader.append(test);

    fileHeader->appendRow(ObtenirModeleOptionalHeader());

    // End FileHeader Column
    fileHeader->appendColumn(descriptionFileHeader);

    return fileHeader;
}

QStandardItem* ClDocument::ObtenirModeleOptionalHeader()
{
    // OptionalHeader
    QStandardItem * optionalHeader = new QStandardItem("Optional Header");
    QList<QStandardItem *> descriptionOptionalHeader;

    optionalHeader->appendRow(new QStandardItem("Entry point"));

    std::ostringstream oss;
    oss.str("");
    oss << std::hex << "0x" << m_pImageOptionalHeader->AddressOfEntryPoint;
    descriptionOptionalHeader.append(new QStandardItem(oss.str().c_str()));

    optionalHeader->appendRow(new QStandardItem("Image base"));
    oss.str("");
    oss << std::hex << "0x" << m_pImageOptionalHeader->ImageBase;
    descriptionOptionalHeader.append(new QStandardItem(oss.str().c_str()));

    optionalHeader->appendRow(new QStandardItem("File alignement"));
    oss.str("");
    oss << std::hex << m_pImageOptionalHeader->FileAlignment;
    descriptionOptionalHeader.append(new QStandardItem(oss.str().c_str()));

    optionalHeader->appendRow(new QStandardItem("Section alignement"));
    oss.str("");
    oss << std::hex <<  m_pImageOptionalHeader->SectionAlignment;
    descriptionOptionalHeader.append(new QStandardItem(oss.str().c_str()));

    optionalHeader->appendColumn(descriptionOptionalHeader);

    return optionalHeader;
}

QStandardItem* ClDocument::ObtenirModeleSectionHeader()
{
    std::ostringstream oss;
    // Section header
    QStandardItem *sectionHeader = new QStandardItem("Section Header");

    //QList<QStandardItem *> descriptionSectionHeader;
    //descriptionSectionHeader.append(new QStandardItem(oss.str().c_str()));
    //sectionHeader->appendColumn(descriptionSectionHeader);

    PIMAGE_SECTION_HEADER pImageSectionHeader;
    for (int i = 0; i != m_pImageFileHeader->NumberOfSections; ++i)
    {
        pImageSectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>((PBYTE)m_pNtHeader + sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*i);

        QStandardItem *tmp = nullptr;
        tmp = new QStandardItem((char *)pImageSectionHeader->Name);

        QList<QStandardItem*> tmpSousItem;
        tmpSousItem.append(new QStandardItem("Pointer to raw :"));
        tmpSousItem.append(new QStandardItem("Virtual address :"));
        tmpSousItem.append(new QStandardItem("Taille :"));

        tmp->appendRows(tmpSousItem);

        QList<QStandardItem*> descriptionSousItem;

        oss.str("");
        oss << "0x" << std::hex << "0x" << pImageSectionHeader->PointerToRawData;

        descriptionSousItem.append(new QStandardItem(oss.str().c_str()));

        oss.str("");
        oss << "0x" << std::hex << "0x" << pImageSectionHeader->VirtualAddress;

        descriptionSousItem.append(new QStandardItem(oss.str().c_str()));

        oss.str("");
        oss << std::hex << "0x" << pImageSectionHeader->SizeOfRawData;

        descriptionSousItem.append(new QStandardItem(oss.str().c_str()));


        tmp->appendColumn(descriptionSousItem);
        sectionHeader->appendRow(tmp);
    }
    // Fin section Header

    return sectionHeader;
}

QStandardItem* ClDocument::ObtenirModeleDataDirectory()
{
    QStandardItem* dataDirectory = new QStandardItem("Data directory");

    for (std::string::size_type i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i)
    {
       QStandardItem* directory = new QStandardItem(DATA_DIRECTORY[i]);

       if (i == 0)
           directory->appendRow(ObtenirModeleExportTable());
       else if (i == 1)
           directory->appendRows(ObtenirModeleImportTable());
       else if (i == 2)
           directory->appendRows(ObtenirModeleRessource());
       else if (i == 3)
           directory->appendRows(ObtenirModeleException());

        dataDirectory->appendRow(directory);
    }

    return dataDirectory;
}

QList<QStandardItem*> ClDocument::ObtenirModeleImportTable()
{
    if (m_pImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0)
    {
        QList<QStandardItem*> tmp;
        tmp.append(new QStandardItem("Aucune fonction importer."));

        return tmp;
    }

    PIMAGE_IMPORT_DESCRIPTOR importTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>((PBYTE)m_pBaseFichier +
                   VirtualAddressToFileAddress(m_pNtHeader, m_pImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress));
    QList<QStandardItem*> it;

    while (importTable->Name != 0)
    {
        QStandardItem* tmp = new QStandardItem(reinterpret_cast<char*>((PBYTE)m_pBaseFichier +
                                                                      VirtualAddressToFileAddress(m_pNtHeader, importTable->Name)));

        PIMAGE_THUNK_DATA fonction = reinterpret_cast<PIMAGE_THUNK_DATA>((PBYTE)m_pBaseFichier +
                                                                         VirtualAddressToFileAddress(m_pNtHeader, importTable->OriginalFirstThunk));

        int cptThunk = 0;

        while (fonction->u1.AddressOfData != 0)
        {
            std::ostringstream oss;

            if (IMAGE_SNAP_BY_ORDINAL(fonction->u1.AddressOfData) == 1)
            {
                oss << "#" << fonction->u1.Ordinal;
                tmp->appendRow(new QStandardItem(oss.str().c_str()));
            }
            else
            {
                PIMAGE_IMPORT_BY_NAME name =
                        reinterpret_cast<PIMAGE_IMPORT_BY_NAME>((PBYTE)m_pBaseFichier +
                                                                VirtualAddressToFileAddress(m_pNtHeader, fonction->u1.AddressOfData));
                oss << name->Name;
                tmp->appendRow(new QStandardItem(oss.str().c_str()));
            }

            ++cptThunk;

            fonction = reinterpret_cast<PIMAGE_THUNK_DATA>((PBYTE)m_pBaseFichier +
                                                           VirtualAddressToFileAddress(m_pNtHeader, importTable->OriginalFirstThunk) + cptThunk*sizeof(IMAGE_THUNK_DATA));
        }
        it.append(tmp);
        importTable++;
    }
    return it;
}

QStandardItem* ClDocument::ObtenirModeleExportTable()
{
    QStandardItem* et;

    if (m_pImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
    {
        et = new QStandardItem("Aucune fonction exporter.");

        return et;
    }

    PIMAGE_EXPORT_DIRECTORY exportTable = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>((PBYTE)m_pBaseFichier +
                            VirtualAddressToFileAddress(m_pNtHeader, m_pImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));

    et = new QStandardItem(reinterpret_cast<char*>((PBYTE)m_pBaseFichier +
                                                   VirtualAddressToFileAddress(m_pNtHeader, exportTable->Name)));
    std::ostringstream oss;
    // Bloc pour préserver le nom des variables étant donné qu'il n'y a pas encore de liste d'initialisation sous vs 2010.

    QList<QStandardItem*> tmp;
    tmp.append(new QStandardItem("Nombre de nom : "));
    QList<QStandardItem*> tmp2;
    oss << exportTable->NumberOfNames;
    tmp2.append(new QStandardItem(oss.str().c_str()));


    DWORD *addresseDesNoms = reinterpret_cast<DWORD *>((PBYTE)m_pBaseFichier +
                                                            VirtualAddressToFileAddress(m_pNtHeader, exportTable->AddressOfNames));
    DWORD *addresseDesFonctions = reinterpret_cast<DWORD *>((PBYTE)m_pBaseFichier +
                                                        VirtualAddressToFileAddress(m_pNtHeader, exportTable->AddressOfFunctions));
    WORD *adresseDesOrdinals = reinterpret_cast<WORD *>((PBYTE)m_pBaseFichier +
                                                        VirtualAddressToFileAddress(m_pNtHeader, exportTable->AddressOfNameOrdinals));

    if (exportTable->NumberOfNames == exportTable->NumberOfFunctions)
        for (int i = 0; i < exportTable->NumberOfNames; ++i)
        {
            oss.str("");
            oss << "#" << std::setw(2) << std::setfill('0') << std::dec << adresseDesOrdinals[i] << " - "
                << reinterpret_cast<char *>((PBYTE)m_pBaseFichier +
                                            VirtualAddressToFileAddress(m_pNtHeader, addresseDesNoms[i]));
            tmp.append(new QStandardItem(oss.str().c_str()));

            oss.str("");

            oss << "Adresse : 0x" << std::hex << std::setw(8) << std::setfill('0') << addresseDesFonctions[i];
            tmp2.append(new QStandardItem(oss.str().c_str()));
        }

    et->appendRows(tmp);
    et->appendColumn(tmp2);

    return et;
}

QList<QStandardItem*> ClDocument::ObtenirModeleRessource()
{
    QList<QStandardItem*> res;

    PIMAGE_RESOURCE_DIRECTORY ressourceDirectory = reinterpret_cast<PIMAGE_RESOURCE_DIRECTORY>((PBYTE)m_pBaseFichier +
                            VirtualAddressToFileAddress(m_pNtHeader, m_pImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress));
    std::ostringstream oss;
    oss << "Nombre d'entrée nommé " << ressourceDirectory->NumberOfNamedEntries;
    res.append(new QStandardItem(oss.str().c_str()));

    oss.str("");
    oss << "Nombre d'entré par ID " << ressourceDirectory->NumberOfIdEntries;
    res.append(new QStandardItem(oss.str().c_str()));

    PIMAGE_RESOURCE_DIRECTORY_ENTRY ressourceEntry = reinterpret_cast<PIMAGE_RESOURCE_DIRECTORY_ENTRY>((PBYTE)ressourceDirectory + sizeof(IMAGE_RESOURCE_DIRECTORY));

    //PIMAGE_RESOURCE_DIRECTORY_ENTRY prochainRessourceEntry = nullptr;

    for (int i = 0; i < ressourceDirectory->NumberOfNamedEntries + ressourceDirectory->NumberOfIdEntries;
        ressourceEntry = reinterpret_cast<PIMAGE_RESOURCE_DIRECTORY_ENTRY>((PBYTE)ressourceDirectory + sizeof(IMAGE_RESOURCE_DIRECTORY)+
                                                                           sizeof(IMAGE_RESOURCE_DIRECTORY) * ++i))
    {
        if (ressourceEntry->Name >= 2147483648) // Ressource avec un nom
        {
            oss.str("");
            oss << "NOM";
            res.append(new QStandardItem(oss.str().c_str()));
        }
        else
        { // Ressource avec un id
            oss.str("");
            oss << "Ressource ID1 : " << ressourceEntry->Name;
            res.append(new QStandardItem(oss.str().c_str()));
            if (ressourceEntry->OffsetToData >= 2147483648)
            {
                // Un autre PIMAGE_RESOURCE_DIRECTORY_ENTRY
                PIMAGE_RESOURCE_DIRECTORY_ENTRY prochainRessourceEntry =
                        reinterpret_cast<PIMAGE_RESOURCE_DIRECTORY_ENTRY>((PBYTE)ressourceDirectory +
                                                                          sizeof(IMAGE_RESOURCE_DIRECTORY) +
                                                                          ressourceEntry->OffsetToData  - (DWORD)2147483648);

                // Si le bit le plus élevé est positioné. (2^31)
                //   Un autre ressource Directory entry
                while (prochainRessourceEntry->OffsetToData >= 2147483648)
                {
                    oss.str("");
                    oss << "Ressource ID : " << prochainRessourceEntry->Name;
                    res.append(new QStandardItem(oss.str().c_str()));

                    prochainRessourceEntry =
                            reinterpret_cast<PIMAGE_RESOURCE_DIRECTORY_ENTRY>((PBYTE)ressourceDirectory +
                                                                              sizeof(IMAGE_RESOURCE_DIRECTORY) +
                                                                              prochainRessourceEntry->OffsetToData  - (DWORD)2147483648);
                    if (prochainRessourceEntry->OffsetToData < 2147483648)
                    {
                        oss.str("");
                        oss << "Ressource ID : " << prochainRessourceEntry->Name;
                        res.append(new QStandardItem(oss.str().c_str()));
                    }
                }
            }
            else
            {
                // IMAGE_RESOURCE_DATA_ENTRY
                oss.str("");
                oss << "Ressource ID : " << ressourceEntry->Name;
                res.append(new QStandardItem(oss.str().c_str()));
            }
        }
    }

    return res;
}

std::vector<ClSection*> ClDocument::Sections() const
{
    return m_sections;
}

QList<QStandardItem*> ClDocument::ObtenirModeleException()
{
    QList<QStandardItem*> exc;

    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY exceptionDirectory = reinterpret_cast<PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY>((PBYTE)m_pBaseFichier +
                            VirtualAddressToFileAddress(m_pNtHeader, m_pImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress));


    /*QList<QStandardItem* > tmp;
    exc.append(new QStandardItem("Adresse de début"));
    std::ostringstream oss;
    oss << exceptionDirectory->BeginAddress;
    tmp.append(new QStandardItem(oss.str().c_str()));

   /* oss.str("");
    oss << "Adresse de fin : " << exceptionDirectory->EndAddress;
    exc.append(new QStandardItem(oss.str().c_str()));

    oss.str("");
    oss << "Adresse d'unwindInfo' : " << exceptionDirectory->UnwindInfoAddress;
    exc.append(new QStandardItem(oss.str().c_str()));*/


    return exc;
}
