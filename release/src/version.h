#pragma once

#include "src/global_config.h"

#include <QtCore/QString>
    
const QString git_commit_hash = "3b7337e"; /*! Current hash of the git branch */
const QString git_branch = "master";      /*! Current name of the branch as printable string    */

#ifdef _UNIX
const QString git_date = "Commit Date: Mon, 22 Jul 2019 22:51:42 +0200"; /*! Current date of the git branch branch as printable string*/
#else
const QString git_date = ""; /*! Current date of the git branch branch as printable string*/
#endif

const QString branch = " Git Branch: master -- ";      /*! Current name of the branch as printable string    */
const QString commit_hash = "Commit: 3b7337e --"; /*! Current hash of the git branch branch as printable string*/

#ifdef _UNIX
const QString date = "Commit Date: Mon, 22 Jul 2019 22:51:42 +0200"; /*! Current date of the git branch branch as printable string*/
#else
const QString date = "Forever young."; /*! Current date of the git branch branch as printable string*/
#endif

#ifdef _DEBUG
const QString conf_mode = "DEBUG Mode"; /*! Last compilation mode used */
#else
const QString conf_mode = "RELEASE Mode"; /*! Last compilation mode used */
#endif

const QString version = QString("Suprafit 2 v-1.6 Alpha \n\t%1 %2 %3 -  %4").arg(branch).arg(commit_hash).arg(date).arg(conf_mode); /*! Version name */

const int qint_version = 1608;
