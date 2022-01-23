/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "src/version.h"

inline QString aboutSFHtml()
{
    QString info;
    info = "<div align='center'><img width='350' src=':/misc/logo_small.png'></div>";
    info += "<h4>" + QString("%1").arg(version).replace("\n", "<br />") + "</h4>";
    info += "<p>This is all about SupraFit, nothing else matters< /p>";
    info += "<p>Created by Conrad Hübler</p>";
    info += "<p>Special thanks to <strong>Prof. M. Mazik</strong>, TU Bergakademie Freiberg for her support.</p>";
    info += "<p>Special thanks to <strong>Dr. Sebastian F&ouml;rster</strong> and <strong>Dr. Stefan Kaiser</strong> for finding bugs and constructive feedback.</p>";
    info += "<p>Special thanks to all with constructive feedback and bug hunting: Felix Amrhein.</p>";
    info += "<p>Special thanks to Alexander K&ouml;nig for testing the Quickstart, helpful comments and feedback.</p>";
    info += "<p>Thanks to all encouraged me writing the application.</p>";
    info += "<p>Built-in Icon Theme taken from Oxygens Icon : <a href='http://www.oxygen-icons.org'>http://www.oxygen-icons.org</a></p>";
    info += "<p>SupraFit website on GitHub: <a href='https://github.com/conradhuebler/SupraFit'>https://github.com/conradhuebler/SupraFit</a></p>";
    info += "<p>If you obtain results with SupraFit, I kindly ask to cite: </p><p>C. Hübler, conradhuebler/SupraFit: 2019, Zenodo. <a href='http://doi.org/10.5281/zenodo.3364569'>http://doi.org/10.5281/zenodo.3364569</a><p>";
    info += "<p>and</p<p>C. Hübler, ChemRxiv 2022, DOI 10.26434/chemrxiv-2022-c1jwr. This content is a preprint and has not been peer-reviewed.</p>";
    info += "<p></p>";
    info += "<p>SupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + ".\n";
#ifdef noto_font
    info += "<p>SupraFit uses and provides some selected Google Noto Font, see <a href='https://github.com/googlei18n/noto-fonts'>https://github.com/googlei18n/noto-fonts</a></p>";
#endif
    return info;
}

inline QString aboutSF()
{
    QString info = QString();
    info += "\t*********************************************************************************************************\n\n";
    info += "\t" + version + "\n";
    info += "\tThis is all about SupraFit, nothing else matters\n";
    info += "\tCreated by Conrad Hübler\n";
    info += "\t*********************************************************************************************************\n\n";
    info += "\tSpecial thanks to Prof. M. Mazik, TU Bergakademie Freiberg for her support.\n\n";
    info += "\tSpecial thanks to \t Dr. Sebastian Förster \t  and \t Dr. Stefan Kaiser \t for finding bugs and constructive feedback.\n";
    info += "\tSpecial thanks to all with constructive feedback and bug hunting: Felix Amrhein.\n";
    info += "\tSpecial thanks to Alexander König for testing the Quickstart, helpful comments and feedback.\n\n\n";
    info += "\tThanks to all encouraged me writing the application.\n\n";
    info += "\tSupraFit website on GitHub: https://github.com/conradhuebler/SupraFit\n\n";
    info += "\tIf you obtain results with SupraFit, I kindly ask to cite: C. Hübler, conradhuebler/SupraFit: 2019, Zenodo. http://doi.org/10.5281/zenodo.3364569\n";
    info += "\tand \n\tC. Hübler, ChemRxiv 2022, DOI 10.26434/chemrxiv-2022-c1jwr. This content is a preprint and has not been peer-reviewed.\n\n";
    info += "\tSupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + ".\n\n";
    info += "\t*********************************************************************************************************\n\n";
    return info;
}
