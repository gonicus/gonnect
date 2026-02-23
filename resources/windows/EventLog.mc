;#ifndef _EXAMPLE_EVENT_LOG_MESSAGE_FILE_H_
;#define _EXAMPLE_EVENT_LOG_MESSAGE_FILE_H_


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
               )

LanguageNames=(English=0x409:MSG00409)

MessageIdTypeDef=DWORD

MessageId=0x100
SymbolicName=MSG_INFO_1
Severity=Informational
Facility=Application
Language=English
%1
.

MessageId=0x101   
SymbolicName=MSG_WARNING_1
Severity=Warning
Facility=Application
Language=English
%1
.

MessageId=0x102
SymbolicName=MSG_ERROR_1
Severity=Error
Facility=Application
Language=English
%1
.

MessageId=0x103
SymbolicName=MSG_SUCCESS_1
Severity=Success
Facility=Application
Language=English
%1
.

;#endif