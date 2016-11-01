# IMPORTFILE

IMPORTFILE allows specification of files from which to read data using the Import() function.



**imName**

Name of IMPORTFILE object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 No             constant

**imFileName=*string***

path name of file to be read. If no path is specified, the file is written in the current directory. If no extension is specified, .csv is used.

  **Units**   **Legal Range**                            **Default**   **Required**   **Variability**
  ----------- ------------------------------------------ ------------- -------------- -----------------
              *file name, path and extension optional*                 No             constant

**imTitle=*string***


If the specified file does not exist, it is created and *xfFileStat* has no effect.

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
              OVERWRITE, NEW, APPEND   OVERWRITE     No             constant

**imFreq=*choice***


If the specified file does not exist, it is created and *xfFileStat* has no effect.

                **Units**   **Legal Range**          **Default**   **Required**   **Variability**
                ----------- ------------------------ ------------- -------------- -----------------
                            OVERWRITE, NEW, APPEND   OVERWRITE     No             constant

**imHeader=*choice***

**imBinary=*choice***

**endImportFile**

Optionally indicates the end of the export file definition. Alternatively, the end of the Export file definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
