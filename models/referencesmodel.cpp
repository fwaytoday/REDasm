#include "referencesmodel.h"
#include "../redasm/disassembler/types/referencetable.h"
#include "../redasm/disassembler/listing/listingdocument.h"
#include "../redasm/plugins/format.h"
#include "../themeprovider.h"

ReferencesModel::ReferencesModel(QObject *parent): DisassemblerModel(parent) { }

void ReferencesModel::setDisassembler(REDasm::DisassemblerAPI *disassembler)
{
    DisassemblerModel::setDisassembler(disassembler);
    m_printer = REDasm::PrinterPtr(disassembler->assembler()->createPrinter(disassembler));
}

void ReferencesModel::clear()
{
    this->beginResetModel();
    m_references.clear();
    this->endResetModel();
}

void ReferencesModel::xref(address_t address)
{
    if(!m_disassembler)
        return;

    this->beginResetModel();
    m_references = m_disassembler->getReferences(address);
    this->endResetModel();
}

QModelIndex ReferencesModel::index(int row, int column, const QModelIndex &) const
{
    if(row >= m_references.size())
        return QModelIndex();

    return this->createIndex(row, column, m_references[row]);
}

QVariant ReferencesModel::data(const QModelIndex &index, int role) const
{
    if(!m_disassembler)
        return QVariant();

    REDasm::ListingDocument* doc = m_disassembler->document();
    auto it = doc->instructionItem(m_references[index.row()]);

    if(it == doc->end())
        it = doc->symbolItem(m_references[index.row()]);

    if(it == doc->end())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        if(index.column() == 0)
            return S_TO_QS(REDasm::hex((*it)->address, m_disassembler->format()->bits()));
        else if(index.column() == 1)
            return this->direction((*it)->address);
        else if(index.column() == 2)
        {
            if((*it)->is(REDasm::ListingItem::InstructionItem))
                return QString::fromStdString(m_printer->out(doc->instruction((*it)->address)));
            else if((*it)->is(REDasm::ListingItem::SymbolItem))
                return QString::fromStdString(m_printer->symbol(doc->symbol((*it)->address)));
        }
    }
    else if(role == Qt::ForegroundRole)
    {
        if(index.column() == 0)
            return QColor(Qt::darkBlue);

        if(index.column() == 2)
        {
            if((*it)->is(REDasm::ListingItem::InstructionItem))
            {
                REDasm::InstructionPtr instruction = doc->instruction((*it)->address);

                if(!instruction->is(REDasm::InstructionTypes::Conditional))
                    return THEME_VALUE("instruction_jmp_c");
                else if(instruction->is(REDasm::InstructionTypes::Jump))
                    return THEME_VALUE("instruction_jmp");
                else if(instruction->is(REDasm::InstructionTypes::Call))
                    return THEME_VALUE("instruction_call");
            }
            else if((*it)->is(REDasm::ListingItem::SymbolItem))
            {
                REDasm::SymbolPtr symbol = doc->symbol((*it)->address);

                if(symbol->is(REDasm::SymbolTypes::Data))
                    return THEME_VALUE("data_fg");
                else if(symbol->is(REDasm::SymbolTypes::String))
                    return THEME_VALUE("string_fg");
            }
        }
    }
    else if((role == Qt::TextAlignmentRole) && (index.column() > 0))
        return Qt::AlignCenter;

    return QVariant();
}

QVariant ReferencesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();

    if(section == 0)
        return "Address";
    else if(section == 1)
        return "Direction";
    else if(section == 2)
        return "Reference";

    return QVariant();
}

int ReferencesModel::rowCount(const QModelIndex &) const { return static_cast<int>(m_references.size()); }
int ReferencesModel::columnCount(const QModelIndex &) const { return 3; }

QString ReferencesModel::direction(address_t address) const
{
    REDasm::ListingCursor* cur = m_disassembler->document()->cursor();
    REDasm::ListingItem* item = m_disassembler->document()->itemAt(cur->currentLine());

    if(address > item->address)
        return "Down";

    if(address < item->address)
        return "Up";

    return "---";
}
