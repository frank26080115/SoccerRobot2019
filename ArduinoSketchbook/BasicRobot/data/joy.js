var touchOrClick = "Unknown";

/*
code borrowed from https://automatedhome.party/2017/07/15/wifi-controlled-car-with-a-self-hosted-htmljs-joystick-using-a-wemos-d1-miniesp8266/
*/

var VirtualJoystick	= function(opts)
{
	opts			= opts			|| {};
	this._container		= opts.container	|| document.body;
	this._strokeStyle	= opts.strokeStyle	|| 'cyan';
	this._stickEl		= opts.stickElement	|| this._buildJoystickStick();
	this._baseEl		= opts.baseElement	|| this._buildJoystickBase();
	this._mouseSupport	= opts.mouseSupport !== undefined ? opts.mouseSupport : false;
	this._stationaryBase	= opts.stationaryBase || false;
	this._baseX		= this._stickX = opts.baseX || 0
	this._baseY		= this._stickY = opts.baseY || 0
	this._limitStickTravel	= opts.limitStickTravel || false
	this._stickRadius	= opts.stickRadius !== undefined ? opts.stickRadius : 100
	this._useCssTransform	= opts.useCssTransform !== undefined ? opts.useCssTransform : false

	this._container.style.position	= "relative"

	this._container.appendChild(this._baseEl)
	this._baseEl.style.position	= "absolute"
	this._baseEl.style.display	= "none"
	this._container.appendChild(this._stickEl)
	this._stickEl.style.position	= "absolute"
	this._stickEl.style.display	= "none"

	this._pressed	= false;
	this._touchIdx	= null;

	if(this._stationaryBase === true){
		this._baseEl.style.display	= "";
		this._baseEl.style.left		= (this._baseX - this._baseEl.width /2)+"px";
		this._baseEl.style.top		= (this._baseY - this._baseEl.height/2)+"px";
	}

	this._transform	= this._useCssTransform ? this._getTransformProperty() : false;
	this._has3d	= this._check3D();

	var __bind	= function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
	this._$onTouchStart	= __bind(this._onTouchStart	, this);
	this._$onTouchEnd	= __bind(this._onTouchEnd	, this);
	this._$onTouchMove	= __bind(this._onTouchMove	, this);
	this._container.addEventListener( 'touchstart'	, this._$onTouchStart	, false );
	this._container.addEventListener( 'touchend'	, this._$onTouchEnd	, false );
	this._container.addEventListener( 'touchmove'	, this._$onTouchMove	, false );
	if( this._mouseSupport ){
		this._$onMouseDown	= __bind(this._onMouseDown	, this);
		this._$onMouseUp	= __bind(this._onMouseUp	, this);
		this._$onMouseMove	= __bind(this._onMouseMove	, this);
		this._container.addEventListener( 'mousedown'	, this._$onMouseDown	, false );
		this._container.addEventListener( 'mouseup'	, this._$onMouseUp	, false );
		this._container.addEventListener( 'mousemove'	, this._$onMouseMove	, false );
	}
}

VirtualJoystick.prototype.destroy	= function()
{
	this._container.removeChild(this._baseEl);
	this._container.removeChild(this._stickEl);

	this._container.removeEventListener( 'touchstart'	, this._$onTouchStart	, false );
	this._container.removeEventListener( 'touchend'		, this._$onTouchEnd	, false );
	this._container.removeEventListener( 'touchmove'	, this._$onTouchMove	, false );
	if( this._mouseSupport ){
		this._container.removeEventListener( 'mouseup'		, this._$onMouseUp	, false );
		this._container.removeEventListener( 'mousedown'	, this._$onMouseDown	, false );
		this._container.removeEventListener( 'mousemove'	, this._$onMouseMove	, false );
	}
}

/**
 * @returns {Boolean} true if touchscreen is currently available, false otherwise
*/
VirtualJoystick.touchScreenAvailable	= function()
{
	return 'createTouch' in document ? true : false;
}

/**
 * microevents.js - https://github.com/jeromeetienne/microevent.js
*/
;(function(destObj){
	destObj.addEventListener	= function(event, fct){
		if(this._events === undefined) 	this._events	= {};
		this._events[event] = this._events[event]	|| [];
		this._events[event].push(fct);
		return fct;
	};
	destObj.removeEventListener	= function(event, fct){
		if(this._events === undefined) 	this._events	= {};
		if( event in this._events === false  )	return;
		this._events[event].splice(this._events[event].indexOf(fct), 1);
	};
	destObj.dispatchEvent		= function(event /* , args... */){
		if(this._events === undefined) 	this._events	= {};
		if( this._events[event] === undefined )	return;
		var tmpArray	= this._events[event].slice();
		for(var i = 0; i < tmpArray.length; i++){
			var result	= tmpArray[i].apply(this, Array.prototype.slice.call(arguments, 1))
			if( result !== undefined )	return result;
		}
		return undefined
	};
})(VirtualJoystick.prototype);

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype.deltaX	= function(){ return this._stickX - this._baseX;	}
VirtualJoystick.prototype.deltaY	= function(){ return this._stickY - this._baseY;	}

VirtualJoystick.prototype.up	= function(){
	if( this._pressed === false )	return false;
	var deltaX	= this.deltaX();
	var deltaY	= this.deltaY();
	if( deltaY >= 0 )				return false;
	if( Math.abs(deltaX) > 2*Math.abs(deltaY) )	return false;
	return true;
}
VirtualJoystick.prototype.down	= function(){
	if( this._pressed === false )	return false;
	var deltaX	= this.deltaX();
	var deltaY	= this.deltaY();
	if( deltaY <= 0 )				return false;
	if( Math.abs(deltaX) > 2*Math.abs(deltaY) )	return false;
	return true;
}
VirtualJoystick.prototype.right	= function(){
	if( this._pressed === false )	return false;
	var deltaX	= this.deltaX();
	var deltaY	= this.deltaY();
	if( deltaX <= 0 )				return false;
	if( Math.abs(deltaY) > 2*Math.abs(deltaX) )	return false;
	return true;
}
VirtualJoystick.prototype.left	= function(){
	if( this._pressed === false )	return false;
	var deltaX	= this.deltaX();
	var deltaY	= this.deltaY();
	if( deltaX >= 0 )				return false;
	if( Math.abs(deltaY) > 2*Math.abs(deltaX) )	return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._onUp	= function()
{
	this._pressed	= false;
	this._stickEl.style.display	= "none";

	if(this._stationaryBase == false){
		this._baseEl.style.display	= "none";

		this._baseX	= this._baseY	= 0;
		this._stickX	= this._stickY	= 0;
	}

	document.getElementsByTagName("body")[0].classList.add("bodybg");
}

VirtualJoystick.prototype._onDown	= function(x, y)
{
	this._pressed	= true;
	if(this._stationaryBase == false){
		this._baseX	= x;
		this._baseY	= y;
		this._baseEl.style.display	= "";
		this._move(this._baseEl.style, (this._baseX - this._baseEl.width /2), (this._baseY - this._baseEl.height/2));
	}

	this._stickX	= x;
	this._stickY	= y;

	if(this._limitStickTravel === true){
		var deltaX	= this.deltaX();
		var deltaY	= this.deltaY();
		var stickDistance = Math.sqrt( (deltaX * deltaX) + (deltaY * deltaY) );
		if(stickDistance > this._stickRadius){
			var stickNormalizedX = deltaX / stickDistance;
			var stickNormalizedY = deltaY / stickDistance;

			this._stickX = stickNormalizedX * this._stickRadius + this._baseX;
			this._stickY = stickNormalizedY * this._stickRadius + this._baseY;
		}
	}

	this._stickEl.style.display	= "";
	this._move(this._stickEl.style, (this._stickX - this._stickEl.width /2), (this._stickY - this._stickEl.height/2));

	document.getElementsByTagName("body")[0].classList.remove("bodybg");
}

VirtualJoystick.prototype._onMove	= function(x, y)
{
	if( this._pressed === true ){
		this._stickX	= x;
		this._stickY	= y;

		if(this._limitStickTravel === true){
			var deltaX	= this.deltaX();
			var deltaY	= this.deltaY();
			if (deltaX > this._stickRadius) {
				this._stickX = this._stickRadius + this._baseX;
			}
			if (deltaY > this._stickRadius) {
				this._stickY = this._stickRadius + this._baseY;
			}
			if (deltaX < -this._stickRadius) {
				this._stickX = this._baseX - this._stickRadius;
			}
			if (deltaY < -this._stickRadius) {
				this._stickY = this._baseY - this._stickRadius;
			}
			/*
			var stickDistance = Math.sqrt( (deltaX * deltaX) + (deltaY * deltaY) );
			if(stickDistance > this._stickRadius){
				var stickNormalizedX = deltaX / stickDistance;
				var stickNormalizedY = deltaY / stickDistance;

				this._stickX = stickNormalizedX * this._stickRadius + this._baseX;
				this._stickY = stickNormalizedY * this._stickRadius + this._baseY;
			}
			*/
		}

			this._move(this._stickEl.style, (this._stickX - this._stickEl.width /2), (this._stickY - this._stickEl.height/2));

			document.getElementsByTagName("body")[0].classList.remove("bodybg");
	}
}


//////////////////////////////////////////////////////////////////////////////////
//              bind touch events (and mouse events for debug)                  //
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._onMouseUp	= function(event)
{
	return this._onUp();
}

VirtualJoystick.prototype._onMouseDown	= function(event)
{
	touchOrClick = "Mouse";
	event.preventDefault();
	var x	= event.clientX;
	var y	= event.clientY;
	return this._onDown(x, y);
}

VirtualJoystick.prototype._onMouseMove	= function(event)
{
	var x	= event.clientX;
	var y	= event.clientY;
	return this._onMove(x, y);
}

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._onTouchStart	= function(event)
{
	touchOrClick = "Touch";
	// if there is already a touch inprogress
	if( this._touchIdx !== null )
	{
		var touchList = event.changedTouches;
		var i;
		for (i = 0; i < touchList.length ; i++ )
		{
			if (this._touchIdx != touchList[i].identifier)
			{
				var possibleTouch = event.changedTouches[i];
				var pX = possibleTouch.pageX;
				var pY = possibleTouch.pageY;
				onMultiTouch(pX, pY);
			}
		}
		return; // do nothing
	}

	// notify event for validation
	var isValid	= this.dispatchEvent('touchStartValidation', event);
	if( isValid === false )	return;

	// dispatch touchStart
	this.dispatchEvent('touchStart', event);

	event.preventDefault();
	// get the first who changed
	var touch	= event.changedTouches[0];
	// set the touchIdx of this joystick
	this._touchIdx	= touch.identifier;

	// forward the action
	var x		= touch.pageX;
	var y		= touch.pageY;
	return this._onDown(x, y)
}

VirtualJoystick.prototype._onTouchEnd	= function(event)
{
	// if there is no touch in progress, do nothing
	if( this._touchIdx === null )	return;

	// dispatch touchEnd
	this.dispatchEvent('touchEnd', event);

	// try to find our touch event
	var touchList	= event.changedTouches;
	for(var i = 0; i < touchList.length && touchList[i].identifier !== this._touchIdx; i++);
	// if touch event isnt found,
	if( i === touchList.length)	return;

	// reset touchIdx - mark it as no-touch-in-progress
	this._touchIdx	= null;

//??????
// no preventDefault to get click event on ios
event.preventDefault();

	return this._onUp()
}

VirtualJoystick.prototype._onTouchMove	= function(event)
{
	// if there is no touch in progress, do nothing
	if( this._touchIdx === null )	return;

	// try to find our touch event
	var touchList	= event.changedTouches;
	var i;
	for (i = 0; i < touchList.length && touchList[i].identifier !== this._touchIdx; i++ );
	// if touch event with the proper identifier isnt found, do nothing
	if( i === touchList.length)	return;
	var touch	= touchList[i];

	event.preventDefault();

	var x		= touch.pageX;
	var y		= touch.pageY;
	return this._onMove(x, y)
}


//////////////////////////////////////////////////////////////////////////////////
//          build default stickEl and baseEl                                    //
//////////////////////////////////////////////////////////////////////////////////

/**
 * build the canvas for joystick base
 */
VirtualJoystick.prototype._buildJoystickBase	= function()
{
	var canvas	= document.createElement( 'canvas' );
	canvas.width	= 126;
	canvas.height	= 126;

	var ctx		= canvas.getContext('2d');
	ctx.beginPath();
	ctx.strokeStyle = this._strokeStyle;
	ctx.lineWidth	= 6;
	ctx.arc( canvas.width/2, canvas.width/2, 40, 0, Math.PI*2, true);
	ctx.stroke();

	ctx.beginPath();
	ctx.strokeStyle	= this._strokeStyle;
	ctx.lineWidth	= 2;
	ctx.arc( canvas.width/2, canvas.width/2, 60, 0, Math.PI*2, true);
	ctx.stroke();

	return canvas;
}

/**
 * build the canvas for joystick stick
 */
VirtualJoystick.prototype._buildJoystickStick	= function()
{
	var canvas	= document.createElement( 'canvas' );
	canvas.width	= 86;
	canvas.height	= 86;
	var ctx		= canvas.getContext('2d');
	ctx.beginPath();
	ctx.strokeStyle	= this._strokeStyle;
	ctx.lineWidth	= 6;
	ctx.arc( canvas.width/2, canvas.width/2, 40, 0, Math.PI*2, true);
	ctx.stroke();
	return canvas;
}

//////////////////////////////////////////////////////////////////////////////////
//		move using translate3d method with fallback to translate > 'top' and 'left'
//      modified from https://github.com/component/translate and dependents
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._move = function(style, x, y)
{
	if (this._transform) {
		if (this._has3d) {
			style[this._transform] = 'translate3d(' + x + 'px,' + y + 'px, 0)';
		} else {
			style[this._transform] = 'translate(' + x + 'px,' + y + 'px)';
		}
	} else {
		style.left = x + 'px';
		style.top = y + 'px';
	}
}

VirtualJoystick.prototype._getTransformProperty = function()
{
	var styles = [
		'webkitTransform',
		'MozTransform',
		'msTransform',
		'OTransform',
		'transform'
	];

	var el = document.createElement('p');
	var style;

	for (var i = 0; i < styles.length; i++) {
		style = styles[i];
		if (null != el.style[style]) {
			return style;
		}
	}
}

VirtualJoystick.prototype._check3D = function()
{
	var prop = this._getTransformProperty();
	// IE8<= doesn't have `getComputedStyle`
	if (!prop || !window.getComputedStyle) return module.exports = false;

	var map = {
		webkitTransform: '-webkit-transform',
		OTransform: '-o-transform',
		msTransform: '-ms-transform',
		MozTransform: '-moz-transform',
		transform: 'transform'
	};

	// from: https://gist.github.com/lorenzopolidori/3794226
	var el = document.createElement('div');
	el.style[prop] = 'translate3d(1px,1px,1px)';
	document.body.insertBefore(el, null);
	var val = getComputedStyle(el).getPropertyValue(map[prop]);
	document.body.removeChild(el);
	var exports = null != val && val.length && 'none' != val;
	return exports;
}

// main code below

console.log("touchscreen is", VirtualJoystick.touchScreenAvailable() ? "available" : "not available");

var joystick	= new VirtualJoystick({
	container	: document.getElementById('container'),
	mouseSupport	: true,
	limitStickTravel	: true,
	stickRadius	: desiredStickRadius,
});
joystick.addEventListener('touchStart', function(){
	console.log('touchStart');
	//document.getElementsByTagName("body")[0].classList.remove("bodybg");
})
joystick.addEventListener('touchEnd', function(){
	console.log('touchEnd');
	//document.getElementsByTagName("body")[0].classList.add("bodybg");;
})

var prevX = 0;
var prevY = 0;
var prevW = 0;
var newX = 0;
var newY = 0;
var xDeadzone = desiredStickRadius / 6;
var yDeadzone = xDeadzone / 2;

var weapspeed = 0;
var flipped = 0;

var touchOrClick = "Unknown";

var startRobot = false;

var xhrTime = new Date();

setInterval(function()
{
	var outputEl	= document.getElementById('result');

	var timeNow = new Date();
	if ((timeNow.getTime() - xhrTime.getTime()) > 2000) {
		robotTimedOut(true);
	}

	newX = Math.round(joystick.deltaX());
	newY = Math.round(joystick.deltaY()) * -1;

	var reportedX = newX;
	var reportedY = newY;

	if (reportedX > xDeadzone) {
		reportedX = reportedX - xDeadzone;
	}
	else if (reportedX < (-xDeadzone)) {
		reportedX = reportedX + xDeadzone;
	}
	else {
		reportedX = 0;
	}
	reportedX = reportedX * desiredStickRadius;
	reportedX = reportedX / (desiredStickRadius - xDeadzone);

	if (reportedY > yDeadzone) {
		reportedY = reportedY - yDeadzone;
	}
	else if (reportedY < (-yDeadzone)) {
		reportedY = reportedY + yDeadzone;
	}
	else {
		reportedY = 0;
	}
	reportedY = reportedY * desiredStickRadius;
	reportedY = reportedY / (desiredStickRadius - yDeadzone);

	reportedX = Math.round(reportedX);
	reportedY = Math.round(reportedY);

	if (outputEl) {
		outputEl.innerHTML	= '<b>' + touchOrClick + ':</b> ' + ' X:'+reportedX + ' Y:'+reportedY;
	}

	if ( newX != prevX || newY != prevY || weapspeed != prevW || weapPosSafe > 0 || startRobot == false)
	{
		var xhr = new XMLHttpRequest();

		var querystring;
		querystring = "./move?";
		if (startRobot)
		{
			querystring += "x="+reportedX+"&y="+reportedY;
			if (advancedFeatures >= 1) {
				querystring += "&flipped=" + flipped;
			}
			if (advancedFeatures >= 2) {
				querystring += "&weap=" + weapspeed;
			}
		}
		else
		{
			querystring += "standby=true";
		}

		xhr.open('PUT', querystring);
		xhr.timeout = 2000;

		xhr.onreadystatechange = function () {
			var doneStatus = 4;
			if (xhr.DONE != undefined) {
				doneStatus = xhr.DONE;
			}
			else if (READYSTATE_COMPLETE != undefined) {
				doneStatus = READYSTATE_COMPLETE;
			}
			else if (xhr.READYSTATE_COMPLETE != undefined) {
				doneStatus = xhr.READYSTATE_COMPLETE;
			}
			if ((xhr.readyState == doneStatus || xhr.readyState === "complete") && xhr.status === 200) {
				var jsonObj = JSON.parse(xhr.responseText);
				handleJson(jsonObj);
				robotTimedOut(false);
				xhrTime = new Date();
			}
		}

		xhr.ontimeout = function () {
			robotTimedOut(true);
		}

		if (window.location.protocol != "file:") {
			xhr.send();
		}
		else {
			console.log("xhr: " + querystring);
		}
	}
	prevX = newX;
	prevY = newY;
	if (advancedFeatures) {
		prevW = weapspeed;
	}
}, 1/30 * 1000);

function startrobot()
{
	var eleBtn = document.getElementById("standby");
	eleBtn.outerHTML = ""; // remove the whole div
	startRobot = true;
	attachMultiTouchEventsToButtons();
	robotTimedOut(false);
}

function robotTimedOut(x)
{
	var ele = document.getElementById('connstat');
	if (x === false)
	{
		ele.innerHTML = "&nbsp;";
	}
	else
	{
		ele.innerHTML = "<br />(CONNECTION LOST)";
	}
}

function doOnOrientationChange()
{
	console.log("onrotate");
	doOnResize();
	/*
	switch(window.orientation) {
	  case -90 || 90:
		changeToLandscape();
		break; 
	  default:
		changeToPortrait();
		break; 
	}
	*/
}

function doOnResize()
{
	console.log("onresize " + window.innerWidth + " x " + window.innerHeight);
	if (window.innerWidth >= window.innerHeight) {
		changeToLandscape();
	}
	else {
		changeToPortrait();
	}
}

function changeToLandscape()
{
	document.getElementById("sidebar").style.display = "block";
	document.getElementById("topbar").style.display = "none";
}

function changeToPortrait()
{
	document.getElementById("sidebar").style.display = "none";
	document.getElementById("topbar").style.display = "block";
}

window.addEventListener('orientationchange', doOnOrientationChange);
window.addEventListener("resize", doOnResize);
doOnResize();

function onMultiTouch(x, y)
{
	console.log("Multitouch X " + x + " Y " + y);
	var inputElements = document.getElementsByTagName("input");
	var i;
	for (i = 0; i < inputElements.length; i++)
	{
		var ele = inputElements[i];
		var box = ele.getBoundingClientRect();
		if (x > box.left && x < box.right && ((box.bottom < box.top && y > box.bottom && y < box.top) || (box.bottom > box.top && y < box.bottom && y > box.top)))
		{
			ele.onclick.apply(ele);
			return;
		}
	}
}

function attachMultiTouchEventsToButtons()
{
	var inputElements = document.getElementsByTagName("input");
	var i;
	for (i = 0; i < inputElements.length; i++)
	{
		var ele = inputElements[i];
		ele.addEventListener( 'touchstart' , function() {
			if (joystick._pressed === true) {
				console.log("touchstart for button");
				this.onclick.apply(this);
			}
		});
	}
}

function weapsetpossafe()
{
	console.log("weapsetpossafe");
	if (advancedFeatures >= 2) {
		weapspeed = weapPosSafe;
	}
}

function weapsetposa()
{
	console.log("weapsetposa");
	if (advancedFeatures >= 2) {
		weapspeed = weapPosA;
	}
}

function weapsetposb()
{
	console.log("weapsetposb");
	if (advancedFeatures >= 2) {
		weapspeed = weapPosB;
	}
}

function flip()
{
	console.log("flip " + flipped);
	if (advancedFeatures <= 0) {
		return;
	}
	var ele1 = document.getElementById("flip1");
	var ele2 = document.getElementById("flip2");
	if (flipped == 0) {
		flipped = 1;
		ele1.checked = true;
		ele2.checked = true;
	}
	else {
		flipped = 0;
		ele1.checked = false;
		ele2.checked = false;
	}
}

function handleJson(jsonObj)
{
	var divEle = document.getElementById("batt");
	if (divEle == undefined || divEle == null) {
		return;
	}
	var warn = false;
	var battVolt = 0;
	var overSymbol = "";
	if (jsonObj.battWarning != undefined) {
		if (jsonObj.battWarning === true || jsonObj.battWarning == "true") {
			warn = true;
		}
	}
	if (jsonObj.battOver != undefined) {
		if (jsonObj.battOver === true || jsonObj.battOver == "true") {
			overSymbol = "over ";
		}
	}
	if (jsonObj.battVoltage != undefined) {
		battVolt = jsonObj.battVoltage;
		battVolt /= 10.0;
		battVolt = Math.round(battVolt);
		battVolt /= 100.0;
	}
	var msg;
	if (warn == false) {
		msg = "Batt Voltage: " + overSymbol + battVolt.toString() + "V";
	}
	else {
		msg = "LOW BATT: " + battVolt.toString() + "V";
	}
	divEle.innerHTML = msg;
}

if (advancedFeatures >= 2) {
	weapsetpossafe();
}