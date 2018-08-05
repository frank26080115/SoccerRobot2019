function validateform(fieldId)
{
	var ele = document.getElementById(fieldId);
	if (ele.tagName == "input")
	{
		var inputType = ele.getAttribute("type");
		if (inputType == "text")
		{
			var txt = ele.value;
			if (txt.length > 0)
			{
				return true;
			}
			else
			{
				alert("Input error! Cannot be blank!");
				return false;
			}
		}
		else if (inputType == "number")
		{
			var txt = ele.value;
			if ((typeof txt) == "number")
			{
				return true;
			}
			else ((typeof txt) == "string")
			{
				if (ele.id == "password")
				{
					var tooShort = false;
					var tooLong = false;
					var illegalChars = false;
					var illegalStr = "";
					if (txt.length < 8) {
						tooShort = true;
					}
					if (txt.length > 31) {
						tooLong = true;
					}
					var sidx = 0;
					for (sidx = 0; sidx < txt.length; sidx++)
					{
						var ccode = txt.charCodeAt(sidx);
						if (ccode < 32 || ccode > 126) {
							illegalChars = true;
							if (illegalStr.length > 0) {
								illegalStr += ", ";
							}
							illegalStr += "'" + txt.charAt(sidx) + "'(0x" + ("0"+(Number(ccode).toString(16))).slice(-2).toUpperCase() + ") ";
						}
					}
					if (tooShort === true || tooLong === true || illegalChars === true) {
						var badPasswordMsg = "Invalid Password! ";
						if (tooShort === true) {
							badPasswordMsg += "Too short (cannot be less than 8 characters long). "
						}
						if (tooLong === true) {
							badPasswordMsg += "Too long (cannot be more than 31 characters long). "
						}
						if (illegalChars === true) {
							badPasswordMsg += "Contains a character that is not allowed: " + illegalStr
						}
						alert(badPasswordMsg);
						return false
					}
				}
				else if (txt.length > 0)
				{
					return true;
				}
				else
				{
					alert("Input error! Cannot be blank!");
					return false;
				}
			}
		}
	}
	else if (ele.tagName == "select")
	{
		return true;
	}
	return true;
}

function validateHexblob()
{
	// TODO
	return true;
}
