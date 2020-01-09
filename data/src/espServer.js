function AjaxLoaderPosition() {
    var WinHeight = 0;
    var WinWidth = 0;
    var ScrollTop = 0;
    var ScrollLeft = 0;
    var conHeight = 0;
    var conWidth = 0;
    WinHeight = parent.window.innerHeight || window.innerHeight || parent.document.documentElement.clientHeight || document.documentElement.clientHeight || parent.document.body.clientHeight || document.body.clientHeight;
    WinWidth = parent.window.innerWidth || window.innerWidth || parent.document.documentElement.clientWidth || document.documentElement.clientWidth || parent.document.body.clientWidth || document.body.clientWidth;
    if (parent.window.pageYOffset || window.pageYOffset || parent.document.documentElement.scrollTop || document.documentElement.scrollTop > 0) {
        ScrollTop = parent.window.pageYOffset || window.pageYOffset || parent.document.documentElement.scrollTop || document.documentElement.scrollTop;
    }
    if (parent.window.pageXOffset || window.pageXOffset || parent.document.documentElement.scrollLeft || document.documentElement.scrollLeft > 0) {
        ScrollLeft = parent.window.pageXOffset || window.pageXOffset || parent.document.documentElement.scrollLeft || document.documentElement.scrollLeft;
    }
    var ScreenDiv = document.getElementById("divScreen");
    if (ScreenDiv) {
        ScreenDiv.style.opacity = "0.3";
        conHeight = ScreenDiv.clientHeight;
        conWidth = ScreenDiv.clientWidth;
    }
    var AjaxLoaderDiv = document.getElementById("divAjaxLoader");
    var AjaxLoaderImage = document.getElementById('imgAjaxLoader');
    var aliHeight = AjaxLoaderImage.naturalHeight;
    var aliWidth = AjaxLoaderImage.naturalWidth;
    aliTop = Math.floor(WinHeight / 2) + ScrollTop - Math.floor(aliHeight / 2);
    aliLeft = Math.floor(WinWidth / 2) + ScrollLeft - Math.floor(aliWidth / 2);
    if (aliTop + aliHeight > conHeight) {
        aliTop = conHeight - aliHeight;
    }
    if (aliLeft + aliWidth > conWidth) {
        aliTop = conWidth - aliWidth;
    }
    AjaxLoaderDiv.style.top = aliTop + 'px';
    AjaxLoaderDiv.style.left = aliLeft + 'px';
}

function UpdateImg(ctrl, imgsrc) {
    var img = document.getElementById(ctrl);
    img.src = imgsrc;
}

function focusField(fieldID, ScrollX, ScrollY) {
    var ff = document.getElementById(fieldID);
    if (ff) {
        ff.focus();
    }
    window.scrollTo(ScrollX, ScrollY);
}

function __doPostBack(eventTarget, eventArgument) {
    myForm.__EVENTTARGET.value = eventTarget;
    myForm.__EVENTARGUMENT.value = eventArgument;
    //var ScrollTop = 0;
    //var ScrollLeft = 0;
    //if (parent.window.pageYOffset || window.pageYOffset || parent.document.documentElement.scrollTop || document.documentElement.scrollTop > 0) {
    //    ScrollTop = parent.window.pageYOffset || window.pageYOffset || parent.document.documentElement.scrollTop || document.documentElement.scrollTop;
    //}
    //if (parent.window.pageXOffset || window.pageXOffset || parent.document.documentElement.scrollLeft || document.documentElement.scrollLeft > 0) {
    //    ScrollLeft = parent.window.pageXOffset || window.pageXOffset || parent.document.documentElement.scrollLeft || document.documentElement.scrollLeft;
    //}
    //myForm.__SCROLL_X.value = ScrollLeft;
    //myForm.__SCROLL_Y.value = ScrollTop;
    var ScreenDiv = document.getElementById("divScreen");
    AjaxLoaderPosition();
    if (ScreenDiv) {
        ScreenDiv.style.opacity = "0.3";
    }
    var AjaxLoaderDiv = document.getElementById("divAjaxLoader");
    if (AjaxLoaderDiv) {
        // AjaxLoaderDiv.innerHTML = '<img id="imgAjaxLoader" src="img/ajax_loader.gif" width="36" height="36" alt="" />';
        AjaxLoaderDiv.style.display = "";
    }
    myForm.submit();
}