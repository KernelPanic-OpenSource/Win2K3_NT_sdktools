<%

	Dim L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE
	Dim L_COMMENTS_NO_RECORDS_MESSAGE
	Dim L_COMMENTS_EVENT_DESCRIPTION_TEXT
	Dim L_COMMENTS_EVENT_NAME_TEXT
	Dim L_COMMENTS_OPERATING_SYSTEM_TEXT
	Dim L_COMMENTS_OPTIONAL_COMMENTS_TEXT
	Dim L_COMMENTS_SAVE_INFO_TEXT
	Dim L_COMMENTS_OPERATING_SYSTEM_MESSAGE
	Dim L_COMMENTS_FILE_NAME_TEXT
	Dim L_COMMENTS_CONNECTION_FAILED_TEXT 
	Dim L_COMMENTS_DATABASE_FAILED_TEXT
	Dim L_COMMENTS_DATABASE_FAILURE_ERRORMESSAGE
	Dim L_COMMENTS_DATABASE_FAILUREBODY_ERRORMESSAGE
	Dim L_COMMENTS_REPORTSAVE_TITLE_TEXT
	Dim L_COMMENTS_REPORTSAVE_BODY_TEXT
	Dim L_COMMENTS_REPORTSAVE_BODYTWO_TEXT
	Dim L_COMMENTS_NOCHANGE_DESCRIPTION_TEXT

	Dim L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX
	Dim L_DETAILS_STATUS_BODY_TEXT
	Dim L_RECEIVED_STATUS_LINK_TEXT
	Dim L_RECEIVED_NEWFILE_LINK_TEXT
	Dim L_SUBMIT_EVENT_MAINTITLE_TEXT
	Dim L_SUBMIT_EVENT_DESCRIPTION_TEXT
	Dim L_SUBMIT_OPERATING_SYSTEM_TEXT
	Dim L_SUBMIT_RE_PRODUCE_EDITBOX
	Dim L_SUBMIT_COMMENTS_INFO_EDITBOX
	Dim L_SUBMIT_MAX_LENGTH_MESSAGE

	Dim L_SUBMIT_SELECT_WINDOWS2000_PROFESSIONAL_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWS2000_SERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWS2000_ADVANCEDSERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWS2000_DATACENTER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_PERSONAL_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_PROFESSIONAL_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_SERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_ADVANCEDSERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_DATACENTER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_64BIT_GROUPBOX





	'-------------------------comments.asp Page Strings-------------------------
	L_SUBMIT_EVENT_DESCRIPTION_TEXT = "Error report description"
	L_SUBMIT_OPERATING_SYSTEM_TEXT = "Operating system:"
	L_SUBMIT_RE_PRODUCE_EDITBOX = "Steps to reproduce the Stop error (optional):"
	L_SUBMIT_COMMENTS_INFO_EDITBOX = "Comments (optional):"
	L_DETAILS_STATUS_BODY_TEXT = "Return to error report status"
	L_SUBMIT_MAX_LENGTH_MESSAGE = "The maximum number of characters is 1024."
	L_SUBMIT_EVENT_MAINTITLE_TEXT = "Selected error report"
	L_RECEIVED_NEWFILE_LINK_TEXT = "Submit an error report"
	L_RECEIVED_STATUS_LINK_TEXT = "Error report status"
	L_COMMENTS_OPERATING_SYSTEM_MESSAGE = "Select the operating system type."
	L_COMMENTS_SAVE_INFO_TEXT = "Update error report"
	L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE = "Database is not available"
	L_COMMENTS_NO_RECORDS_MESSAGE = "No error reports were found. To submit an error report, click Submit Report."
	L_COMMENTS_EVENT_DESCRIPTION_TEXT = "Error report description"
	L_COMMENTS_EVENT_NAME_TEXT = "Description:"
	L_COMMENTS_FILE_NAME_TEXT = "Comments for error report: "
	L_COMMENTS_CONNECTION_FAILED_TEXT = "Windows Online Crash Analysis was unable to connect to the database. Please try this task again." 
	L_COMMENTS_DATABASE_FAILED_TEXT = "Windows Online Crash Analysis was unable to save your information to the database. Please try this task again."
	L_COMMENTS_DATABASE_FAILURE_ERRORMESSAGE = "Database is not available"
	L_COMMENTS_DATABASE_FAILUREBODY_ERRORMESSAGE = "Windows Online Crash Analysis was unable to save your information to the database. Please try this task again. "
	L_COMMENTS_REPORTSAVE_TITLE_TEXT = "Error report updated"
	L_COMMENTS_REPORTSAVE_BODY_TEXT = "Error report "
	L_COMMENTS_REPORTSAVE_BODYTWO_TEXT = " was successfully updated."
	L_COMMENTS_NOCHANGE_DESCRIPTION_TEXT = "You can change the settings and text for every field except Description."
	L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX = "- Select system -"
	L_SUBMIT_SELECT_WINDOWS2000_PROFESSIONAL_GROUPBOX = "Windows 2000 Professional"
	L_SUBMIT_SELECT_WINDOWS2000_SERVER_GROUPBOX = "Windows 2000 Server"
	L_SUBMIT_SELECT_WINDOWS2000_ADVANCEDSERVER_GROUPBOX = "Windows 2000 Advanced Server"
	L_SUBMIT_SELECT_WINDOWS2000_DATACENTER_GROUPBOX = "Windows 2000 Datacenter Server"
	L_SUBMIT_SELECT_WINDOWSXP_PERSONAL_GROUPBOX = "Windows XP Home Edition"
	L_SUBMIT_SELECT_WINDOWSXP_PROFESSIONAL_GROUPBOX = "Windows XP Professional"
	L_SUBMIT_SELECT_WINDOWSXP_SERVER_GROUPBOX = "Windows Server 2003"
	L_SUBMIT_SELECT_WINDOWSXP_ADVANCEDSERVER_GROUPBOX = ""
	L_SUBMIT_SELECT_WINDOWSXP_DATACENTER_GROUPBOX = ""
	L_SUBMIT_SELECT_WINDOWSXP_64BIT_GROUPBOX = "Windows XP 64-Bit Edition"
	'-----------------------------------------------------------------------------------------




%>