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
		}
	}
	else if (ele.tagName == "select")
	{
		return true;
	}
	return true;
}