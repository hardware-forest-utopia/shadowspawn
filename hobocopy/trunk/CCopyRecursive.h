/* 
Copyright (c) 2006 Wangdera Corporation (hobocopy@wangdera.com)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

class CCopyRecursive : public CRecursiveAction
{
private: 
    LONGLONG _byteCount; 
    LPCTSTR _destination; 
    int _directoryCount; 
    int _fileCount; 
    CCopyFilter& _filter; 
    int _skipCount; 
    bool _skipDenied; 
    LPCTSTR _source; 

public: 
    CCopyRecursive::CCopyRecursive(LPCTSTR source, LPCTSTR destination, bool skipDenied, CCopyFilter& filter) : _filter(filter)
    {
        _source = source; 
        _destination = destination; 
        _skipDenied = skipDenied; 
        _fileCount = 0; 
        _directoryCount = 0; 
        _skipCount = 0; 
        _byteCount = 0; 
    }

    LONGLONG get_ByteCount(void)
    {
        return _byteCount; 
    }

    int get_DirectoryCount(void)
    {
        return _directoryCount; 
    }

    int get_FileCount(void)
    {
        return _fileCount; 
    }

    int get_SkipCount(void)
    {
        return _skipCount; 
    }

    void VisitDirectoryFinal(LPCTSTR path)
    {
        CString message; 
        message.AppendFormat(TEXT("Copied directory %s"), path); 
        InstrumentationHelper::Log(message, LOG_LEVEL_INFO); 
        ++_directoryCount; 
    }

    void VisitDirectoryInitial(LPCTSTR path)
    {
        CString destDir;
        Utilities::CombinePath(_destination, path, destDir);
        Utilities::FixLongFilenames(destDir); 

        BOOL bCreated = ::CreateDirectory(destDir, NULL);

        if (!bCreated)
        {
            DWORD error = ::GetLastError(); 

            if (error != ERROR_ALREADY_EXISTS)
            {
                CString errorMessage; 
                Utilities::FormatErrorMessage(error, errorMessage); 
                CString message; 
                message.AppendFormat(TEXT("Creation of directory failed with error %s on directory %s"), 
                    errorMessage, destDir); 
                throw new CHoboCopyException(message);
            }
        }
        else
        {
            CString message; 
            message.AppendFormat(TEXT("Created directory %s"), destDir); 
            InstrumentationHelper::Log(message, LOG_LEVEL_DEBUG); 
        }

    }
    void VisitFile(LPCTSTR path) 
    {
        CString sourceFile;
        Utilities::CombinePath(_source, path, sourceFile);

        CString destinationFile; 
        Utilities::CombinePath(_destination, path, destinationFile);
        Utilities::FixLongFilenames(destinationFile); 
        if (_filter.IsFileMatch(sourceFile))
        {
            BOOL worked = ::CopyFile(sourceFile, destinationFile, false);
            if (!worked)
            {
                DWORD error = ::GetLastError();

                if (error == 5 && _skipDenied)
                {
                    CString message; 
                    message.Format(TEXT("Error accessing file %s. Skipping."), sourceFile); 
                    InstrumentationHelper::Log(message, LOG_LEVEL_WARN); 
                    ++_skipCount; 
                }
                else
                {
                    CString errorMessage; 
                    Utilities::FormatErrorMessage(error, errorMessage); 
                    CString message; 
                    message.AppendFormat(TEXT("Copy of file failed with error %s on file %s"), 
                        errorMessage, sourceFile); 
                    throw new CHoboCopyException(message);
                }
            }
            else
            {
                CString message; 
                message.AppendFormat(TEXT("Copied file %s to %s"), sourceFile, destinationFile); 
                InstrumentationHelper::Log(message, LOG_LEVEL_DEBUG); 
                ++_fileCount; 

                _byteCount += Utilities::GetFileSize(sourceFile); 
            }
        }
        else
        {
            CString message;
            message.AppendFormat(TEXT("Skipping file %s because it doesn't meet filter criteria."), path); 
            InstrumentationHelper::Log(message); 
            ++_skipCount; 
        }

    }
};