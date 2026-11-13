/************************************************************************
 **
 **  @file   tst_exporttemplate.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le modèle de mise en page PDF : la graisse qu'il écrit, et le schéma qui l'accepte.
 **
 **          Un modèle enregistré est validé contre son schéma avant d'atteindre le disque. Depuis que
 **          la graisse est écrite en NOM de face (« Regular ») et non plus en nombre (400), le schéma
 **          la déclarait toujours `xs:int` : tout modèle dont la police avait été choisie refusait de
 **          s'enregistrer. Ce que ces cas fixent : l'écrivain et le schéma disent la même chose, un
 **          ancien fichier qui porte encore le nombre reste valide, et le schéma n'a pas été ouvert
 **          au point de tout accepter.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "tst_exporttemplate.h"

#include "ifc/xml/vdomdocument.h"
#include "oexporttemplate.h"
#include "oexporttemplatestorage.h"
#include "vmisc/testvapplication.h"
#include "vmisc/vmainbase.h"

#include <QDomDocument>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
// The schema the save path holds a template to (OExportTemplateStorage::SaveSchema), named here so that
// what this test validates is exactly what the file on disk is validated against.
const QString kSchema = QStringLiteral("://schema/assemblymark_template/v1.0.0.xsd");

//---------------------------------------------------------------------------------------------------------------------
// A template with a page number at the centre of the sheet, written in one face — the state the editor
// leaves it in as soon as a font has been chosen. The face name is what the writer stores.
auto MakeTemplate(const QString &face) -> OExportTemplate
{
    OExportTemplate tmpl;
    tmpl.name = QStringLiteral("Test");
    tmpl.pagination.centre.enabled = true;
    tmpl.pagination.centre.style.fontFamily = QStringLiteral("Inter");
    tmpl.pagination.centre.style.fontFace = face;
    return tmpl;
}

//---------------------------------------------------------------------------------------------------------------------
auto Validate(const QDomDocument &doc, QString &error) -> bool
{
    return VDomDocument::ValidateXMLData(kSchema, doc.toByteArray(4), QStringLiteral("Test.oamt"), error);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_ExportTemplate::TST_ExportTemplate(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_ExportTemplate::AFaceNameIsAValidTemplate()
{
    QDomDocument doc;
    const OExportTemplate tmpl = MakeTemplate(QStringLiteral("Regular"));
    doc.appendChild(tmpl.ToXml(doc));

    // The face travels in `fontWeight`, and as a NAME: the attribute kept the name it had when it held
    // the numeric weight, so a template written then still reads here.
    QVERIFY2(doc.toString().contains(QStringLiteral("fontWeight=\"Regular\"")), qPrintable(doc.toString()));

    QString error;
    QVERIFY2(Validate(doc, error), qPrintable(error));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_ExportTemplate::AnOldNumberIsStillAValidTemplate()
{
    // A template written before the faces were named holds 400 in that attribute. It has to keep
    // opening, and it keeps the face it was drawn in: FillFontFaces resolves the number.
    QDomDocument doc;
    const OExportTemplate tmpl = MakeTemplate(QStringLiteral("400"));
    doc.appendChild(tmpl.ToXml(doc));

    QString error;
    QVERIFY2(Validate(doc, error), qPrintable(error));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_ExportTemplate::TheFaceSurvivesTheRoundTrip()
{
    QDomDocument doc;
    const OExportTemplate written = MakeTemplate(QStringLiteral("SemiBold"));
    doc.appendChild(written.ToXml(doc));

    QDomDocument parsed;
    QVERIFY2(parsed.setContent(doc.toString()), "the serialised template must be readable XML");

    OExportTemplate read;
    QVERIFY(read.LoadFromXml(parsed.documentElement()));
    QCOMPARE(read.pagination.centre.style.fontFace, QStringLiteral("SemiBold"));
    QCOMPARE(read.pagination.centre.style.fontFamily, QStringLiteral("Inter"));
    QVERIFY(read.pagination.centre.enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_ExportTemplate::TheSchemaStillRefusesAbsurdValues()
{
    // The face is free text now; nothing else is. A size that is not a number is still refused, so
    // the schema was corrected rather than emptied.
    QDomDocument doc;
    const OExportTemplate tmpl = MakeTemplate(QStringLiteral("Regular"));
    doc.appendChild(tmpl.ToXml(doc));

    QDomElement centre = doc.documentElement().firstChildElement(QStringLiteral("pagination"))
                             .firstChildElement(QStringLiteral("centre"));
    QVERIFY2(not centre.isNull(), qPrintable(doc.toString()));
    centre.setAttribute(QStringLiteral("fontSize"), QStringLiteral("big"));

    QString error;
    QVERIFY2(not Validate(doc, error), "a size that is not a number must still be refused");
}

//---------------------------------------------------------------------------------------------------------------------
void TST_ExportTemplate::TheSavePathWritesAFaceName()
{
    // The editor's own path: OExportTemplateStorage::Save, which validates through
    // AssemblyMarkTemplateDocument against `://schema/assemblymark_template/v1.0.0.xsd` and commits with
    // a QSaveFile. This is the call whose refusal the user sees, so it is the one that has to succeed.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("Test.oamt"));

    QString error;
    QVERIFY2(OExportTemplateStorage::Save(MakeTemplate(QStringLiteral("Regular")), path, &error),
             qPrintable(error));

    OExportTemplate read;
    QVERIFY2(OExportTemplateStorage::Load(path, &read, &error), qPrintable(error));
    QCOMPARE(read.pagination.centre.style.fontFace, QStringLiteral("Regular"));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Les schémas XSD sont des ressources compilées avec IFCLib : sans cette ligne, le schéma du modèle
    // est introuvable et la validation échoue pour la mauvaise raison.
    Q_INIT_RESOURCE(schema); // NOLINT
    TestVApplication const app(argc, argv);
    // La validation passe par Xerces, comme l'enregistrement : sans cette initialisation, la
    // bibliothèque tombe sur un gestionnaire de mémoire nul au premier tampon.
    VXercesGuard const xercesGuard;

    TST_ExportTemplate test;
    return QTest::qExec(&test, argc, argv);
}
