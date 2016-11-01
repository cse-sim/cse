# IMPORTFILE

IMPORTFILE allows specification of files from which data is read using the Import() function, allowing external values to be referenced in expressions.

A file suitable for importing has the following structure ...

EXPORT, then IMPORT



**imName**

Name of IMPORTFILE object (for reference from Import()).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 No             constant

**imFileName=*string***

path name of file to be read. If no path is specified, the file located via include.  What is the assumed extension??

  **Units**   **Legal Range**                            **Default**   **Required**   **Variability**
  ----------- ------------------------------------------ ------------- -------------- -----------------
              *file name, path optional*                                Yes             constant

**imTitle=*string***

Expected title found on line 3 of file.  If title does not match, what?

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
               Text string             (blank)       No             constant

**imFreq=*choice***

Specifies the record frequency of the file.

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
              YES NO                                 Yes             constant

**imHeader=*choice***

**Units**   **Legal Range**          **Default**   **Required**   **Variability**
----------- ------------------------ ------------- -------------- -----------------
            YES NO                   YES            No             constant


**imBinary=*choice***

**Units**   **Legal Range**          **Default**   **Required**   **Variability**
----------- ------------------------ ------------- -------------- -----------------
            YES NO                   YES            No             constant

**endImportFile**

Optionally indicates the end of the import file definition. Alternatively, the end of the import file definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
