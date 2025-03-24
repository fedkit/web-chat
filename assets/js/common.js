let dopDopYesYes = (ign) => {};

function sleep(ms){
    return new Promise(res => setTimeout(res, ms));
}

async function apiRequest(type, req){
    let A = await fetch("/api/" + type,
        {method: 'POST', body: JSON.stringify(req)});
    let B = await A.json();
    if (B.status !== 0)
        throw Error("Server returned non-zero status");
    return B;
}

/* Framework for pages with mainloop (it can be npt only polling, but also literally anything else */
let __mainloopDelayMs = 3000;
let mainloopTimeout = null;
let __guestMainloopPollerAction = null;
function setMainloopTimeout(){
    if (mainloopTimeout !== null)
        return;
    mainloopTimeout = setTimeout(mainloopPoller, __mainloopDelayMs);
}
function cancelMainloopTimeout(){
    if (mainloopTimeout === null){
        console.log("cancelling nothing")
        return;
    }
    clearTimeout(mainloopTimeout);
    mainloopTimeout = null;
}
function mainloopPoller(){
    mainloopTimeout = null;
    try {
        if (__guestMainloopPollerAction)
            __guestMainloopPollerAction();
    } catch (error){
        console.log(error)
    }
    setMainloopTimeout();
}

// 1
const userChatRoleAdmin = "admin";
// 2
const userChatRoleRegular = "regular";
// 3
const userChatRoleReadOnly = "read-only";
// 4
const userChatRoleDeleted = "not-a-member";

function roleToColor(role) {
    if (role === userChatRoleAdmin) {
        return "#aafff3";
    } else if (role === userChatRoleRegular){
        return "#ffffff";

    } else if (role === userChatRoleReadOnly){
        return "#bfb2b2";
    } else if (role === userChatRoleDeleted) {
        return "#fb4a4a";
    }
    return "#286500"  // Bug
}

function hideHTMLElement(el){
    el.style.display = "none";
}

function showHTMLElement(el){
    el.style.display = "block";
}

function setElementVisibility(el, isVisible, howVisible = "block"){
    el.style.display = isVisible ? howVisible : "none";
}