/* 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

function addCookie(name, value) {
    var exp = new Date();
    exp.setTime(exp.getTime() + 30 * 60 * 1000);
    document.cookie = name + "=" + escape(value) + ";expires=" + exp.toGMTString() + ";path=/";    
}

function getCookie(name) {
    var arr, reg = new RegExp("(^|)" + name + "=([^;]*)(;|$)");
    if (arr=document.cookie.match(reg)) {
        return unescape(arr[2]);
    } 
}

function delCookie(name) {
    var exp = new Date();
    exp.setTime(exp.getTime() - 1);
    var cval = getCookie(name);
    if (cval != null) {
        document.cookie= name + "=" + cval + ";expires="+exp.toGMTString() + ";path=/";
    }
}

function logout() {
    delCookie("user");
    window.location = "../login.html";
}
