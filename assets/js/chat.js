let LocalHistoryId = 0;

let members = new Map();

let loadedMessages = new Map();  // messageSt objects
/*
container: EL, box: EL, offset: number (msgPres) */
let visibleMessages = new Map();  // HTMLElement objects

let anchoredMsg = -1;
let visibleMsgSegStart = -1;
let visibleMsgSegEnd = -2;
let offsetOfAnchor = 500;
let highestPoint = null;
let lowestPoint = null;

let lastMsgId = -1;
let myRoleHere = null;  // Dung local state updates should be updated first

// Would start with true if opened `/chat/<>`
let bumpedAtBottom = false;

// Hidden variable. When deletion window popup is active
// Persists from popup activation until popup deactivation
let storeHiddenMsgIdForDeletionWin = -1;

let debugMode = false;

// Positive in production, negative for debug
let softZoneSz = debugMode ? -150 : 300;
let chatPadding = debugMode ? 300 : 5;
let msgGap = 5;
const msgErased = pres.chat.msgErased;

function genSentBase(){
    return {
        'chatUpdReq': {
            'LocalHistoryId': LocalHistoryId,
            'chatId': openedchat.id
        }
    };
}

function genSentBaseGMN(){
    let Sent = genSentBase();
    Sent.amount = debugMode ? 2 : 14;
    return Sent;
}

function getChatWgSz(){
    let chatWg = document.getElementById("chat-widget");
    return [chatWg.offsetWidth, chatWg.offsetHeight];
}

function elSetOffsetInChat(el, offset){
    el.style.bottom = String(offset) + "px";
}

function isMissingPrimaryMsgHeap(){
    return lastMsgId >= 0 && anchoredMsg < 0;
}

function isMissingTopMsgHeap(){
    let [W, H] = getChatWgSz();
    return anchoredMsg >= 0 && (highestPoint < H + softZoneSz && visibleMsgSegStart > 0);
}

function isMissingBottomMsgHeap(){
    return anchoredMsg >= 0 && (lowestPoint > - softZoneSz && visibleMsgSegEnd < lastMsgId);
}

function updateOffsetOfVisibleMsg(msgId, offset){
    visibleMessages.get(msgId).container.style.bottom = String(offset) + "px";
}

function updateOffsetsUpToTop(){
    let offset = offsetOfAnchor;
    for (let curMsg = anchoredMsg; curMsg >= visibleMsgSegStart; curMsg--){
        updateOffsetOfVisibleMsg(curMsg, offset);
        let height = visibleMessages.get(curMsg).container.offsetHeight;
        offset += height + msgGap;
    }
    return offset - msgGap;
}

function updateOffsetsDown(){
    let offset = offsetOfAnchor;
    for (let curMsg = anchoredMsg + 1; curMsg <= visibleMsgSegEnd; curMsg++){
        let height = visibleMessages.get(curMsg).container.offsetHeight;
        offset -= (height + msgGap);
        updateOffsetOfVisibleMsg(curMsg, offset);
    }
    return offset;
}

function updateOffsetsSane(){
    if (anchoredMsg < 0)
        return;
    highestPoint = updateOffsetsUpToTop();
    lowestPoint = updateOffsetsDown();
}

function heightOfPreloadGhost(){
    let [W, H] = getChatWgSz();
    return Math.min(H * 0.9, Math.max(H * 0.69, 30));
}

function updateOffsets(){
    let spinnerTop = document.getElementById("top-loading");
    let spinnerBottom = document.getElementById("bottom-loading");
    let SbH = spinnerBottom.offsetHeight;
    if (anchoredMsg < 0){
        hideHTMLElement(spinnerBottom);
        elSetOffsetInChat(spinnerTop, chatPadding);
        setElementVisibility(spinnerTop, isMissingPrimaryMsgHeap());
    } else {
        let [W, H] = getChatWgSz();
        updateOffsetsSane();
        let lowestLowestPoint = isMissingBottomMsgHeap() ? lowestPoint - heightOfPreloadGhost(): lowestPoint;
        let highestHighestPoint = isMissingTopMsgHeap() ? highestPoint + heightOfPreloadGhost() : highestPoint;
        if (lowestLowestPoint > chatPadding || (highestHighestPoint - lowestLowestPoint) <= H - chatPadding * 2 ||
            (!isMissingBottomMsgHeap() && bumpedAtBottom)) {

            offsetOfAnchor += (-lowestLowestPoint + chatPadding);
            updateOffsetsSane();
        } else if (highestHighestPoint < H - chatPadding) {
            offsetOfAnchor += (-highestHighestPoint + (H - chatPadding));
            updateOffsetsSane();
        }
        /* Messages weere updated (and only them). They were talking with ghosts.
        Now we are trying to show spinners of ghosts */
        elSetOffsetInChat(spinnerTop, highestPoint);
        setElementVisibility(spinnerTop, isMissingTopMsgHeap());
        elSetOffsetInChat(spinnerBottom, lowestPoint - SbH);
        setElementVisibility(spinnerBottom, isMissingBottomMsgHeap());
        /* Fix anchor */
        let oldAnchor = anchoredMsg;
        while (true){
            let h = visibleMessages.get(anchoredMsg).container.offsetHeight;
            if (!(offsetOfAnchor + h < chatPadding && visibleMsgSegStart < anchoredMsg))
                break
            offsetOfAnchor += (msgGap + h);
            anchoredMsg--;
        }
        while (offsetOfAnchor > H - chatPadding && anchoredMsg < visibleMsgSegEnd){
            anchoredMsg++;
            let h = visibleMessages.get(anchoredMsg).container.offsetHeight;
            offsetOfAnchor -= (msgGap + h);
        }
        if (oldAnchor !== anchoredMsg)
            console.log("anchoredMsg: " + String(oldAnchor) + " -> " + String(anchoredMsg))
    }
}

function shouldShowDeleteMesgBtn(messageSt){
    return !messageSt.isSystem && messageSt.exists && (myRoleHere !== userChatRoleReadOnly) &&(
        myRoleHere === userChatRoleAdmin || messageSt.senderUserId === userinfo.uid);
}

function getMsgTypeClassSenderBased(messageSt){
    if (messageSt.isSystem)
        return "message-box-system"
    if (messageSt.senderUserId === userinfo.uid)
        return "message-box-mine"
    return "message-box-alien";
}

function getMsgFullTypeClassName(messageSt){
    return getMsgTypeClassSenderBased(messageSt) + (messageSt.exists ? "" : " message-box-deleted");
}

/* Two things can be updated: messages existance and delete button visibility
* Supercontainer.container is persistent, Supercontainer.box can change it's class */
function updateMessageSupercontainer(supercontainer, messageSt){
    let box = supercontainer.box;
    if (messageSt.isSystem)
        return;
    setElementVisibility(box.querySelector(".message-box-button-delete"), shouldShowDeleteMesgBtn(messageSt), "inline");
    box.className = getMsgFullTypeClassName(messageSt);
    // Notice, that no check of previous state is performed. Double loading is a rare event, I can afford to be slow
    if (!messageSt.exists)
        box.querySelector(".message-box-msg").innerText = msgErased;
}

function decodeSystemMessage(text){
    let [subject, verb, object] = text.split(',');
    let subjectId = Number(subject);
    let objectId = Number(object);
    let subjectRef = members.has(subjectId) ? members.get(subjectId).nickname : "???";
    let objectRef = members.has(objectId) ? members.get(objectId).nickname : "???";
    if (verb === "kicked"){
        return subjectRef + " " + pres.chat.syslog.kicked + " " + objectRef;
    } else if (verb === "summoned"){
        return subjectRef + " " + pres.chat.syslog.summoned + " " + objectRef;
    } else if (verb === "left"){
        return subjectRef + " " + pres.chat.syslog.left;
    } else if (verb === "created"){
        return subjectRef + " " + pres.chat.syslog.created;
    }
    return "... Bad log ...";
}

function convertMessageStToSupercontainer(messageSt){
    let container = document.createElement("div");
    container.className = "message-supercontainer";

    let box = document.createElement("div");
    container.appendChild(box);
    box.className = getMsgFullTypeClassName(messageSt);

    let ID = messageSt.id;

    if (messageSt.isSystem){

    } else {
        let topPart = document.createElement("div");
        box.appendChild(topPart);
        topPart.className = "message-box-top";

        if (!members.has(messageSt.senderUserId))
            throw new Error("First - update members");
        let senderMemberSt = members.get(messageSt.senderUserId);
        let senderProfileURI = "/user/" + senderMemberSt.nickname;

        let inTopPartSenderName = document.createElement("a");
        topPart.appendChild(inTopPartSenderName);
        inTopPartSenderName.className = "message-box-sender-name";
        inTopPartSenderName.innerText = senderMemberSt.name;
        inTopPartSenderName.href = senderProfileURI;

        let inTopPartSenderNickname = document.createElement("a");
        topPart.appendChild(inTopPartSenderNickname);
        inTopPartSenderNickname.className = "message-box-sender-name message-box-sender-shortname"
        inTopPartSenderNickname.innerText = senderMemberSt.nickname;
        inTopPartSenderNickname.href = senderProfileURI;

        let inTopPartButtonDelete = document.createElement("img");
        topPart.appendChild(inTopPartButtonDelete);
        inTopPartButtonDelete.className = "message-box-button message-box-button-delete";
        inTopPartButtonDelete.src = "/assets/img/delete.svg";
        inTopPartButtonDelete.onclick = (ev) => {
            if (ev.button !== 0)
                return;
            let msgText = box.querySelector(".message-box-msg").innerText;
            let previewText = senderMemberSt.nickname + ":\n" + msgText;
            if (previewText.length > 1000)
                previewText = previewText.substring(0, 1000 - 3);
            document.getElementById("win-deletion-msg-preview").innerText = previewText;
            storeHiddenMsgIdForDeletionWin = ID;
            activatePopupWindowById("msg-deletion-win");
        };
        setElementVisibility(inTopPartButtonDelete, shouldShowDeleteMesgBtn(messageSt), "inline");

        let inTopPartButtonGetLink = document.createElement("img");
        topPart.appendChild(inTopPartButtonGetLink);
        inTopPartButtonGetLink.className = "message-box-button";
        inTopPartButtonGetLink.src = "/assets/img/link.svg";
        inTopPartButtonGetLink.onclick = (ev) => {
            if (ev.button !== 0)
                return;
            let URI = window.location.host + "/chat/" + openedchat.nickname + "/m/" + String(ID);
            document.getElementById("message-input").innerText += (" " + URI + "");
        };
    }

    let msgPart = document.createElement("p");
    box.appendChild(msgPart);
    msgPart.className = "message-box-msg";
    if (messageSt.exists){
        if (messageSt.isSystem)
            msgPart.innerText = decodeSystemMessage(messageSt.text);
        else
            msgPart.innerText = messageSt.text;
    } else
        msgPart.innerText = msgErased;

    return {'container': container, 'box': box};
}

function makeVisible(msgId){
    let supercontainer = convertMessageStToSupercontainer(loadedMessages.get(msgId));
    const chatWin = document.getElementById("chat-widget");
    chatWin.appendChild(supercontainer.container);
    visibleMessages.set(msgId, supercontainer);
}

function opaNewMessageSt(messageSt){
    let msgId = messageSt.id;
    if (loadedMessages.has(msgId)){
        loadedMessages.set(msgId, messageSt);
        if (visibleMessages.has(msgId)){
            updateMessageSupercontainer(visibleMessages.get(msgId), messageSt);
        }
    } else {
        loadedMessages.set(msgId, messageSt);
        if (anchoredMsg < 0){
            anchoredMsg = msgId;
            visibleMsgSegStart = msgId;
            visibleMsgSegEnd = msgId;
            makeVisible(msgId);
        } else if (msgId + 1 === visibleMsgSegStart) {
            visibleMsgSegStart--;
            makeVisible(msgId);
            while (loadedMessages.has(visibleMsgSegStart - 1)){
                visibleMsgSegStart--;
                makeVisible(visibleMsgSegStart);
            }
        } else if (msgId - 1 === visibleMsgSegEnd){
            visibleMsgSegEnd++;
            makeVisible(msgId);
            while (loadedMessages.has(visibleMsgSegEnd + 1)){
                visibleMsgSegEnd++;
                makeVisible(visibleMsgSegEnd);
            }
        }
    }
}

function canISendMessages(){
    return myRoleHere === userChatRoleRegular || myRoleHere === userChatRoleAdmin;
}

function updateLocalStateFromChatUpdRespBlind(chatUpdResp){
    LocalHistoryId = chatUpdResp.HistoryId;
    for (let memberSt of chatUpdResp.members){
        let id = memberSt.userId;
        if (id === userinfo.uid && myRoleHere !== memberSt.roleHere) {
            myRoleHere = memberSt.roleHere;
            for (let [msgId, sc] of visibleMessages){
                updateMessageSupercontainer(sc, loadedMessages.get(msgId));
            }
            setElementVisibility(document.getElementById("message-input"), canISendMessages());
        }
    }
    for (let memberSt of chatUpdResp.members){
        let id = memberSt.userId;
        members.set(id, memberSt);
    }
    lastMsgId = chatUpdResp.lastMsgId;
    for (let messageSt of chatUpdResp.messages){
        opaNewMessageSt(messageSt);
    }
    updateOffsets();
}

function updateLocalStateFromRecvBlind(Recv){
    updateLocalStateFromChatUpdRespBlind(Recv.chatUpdResp);
}

async function requestMessageNeighbours(fromMsg, direction){
    let Sent = genSentBaseGMN();
    Sent.msgId = fromMsg;
    Sent.direction = direction;
    let Recv = await apiRequest("getMessageNeighbours", Sent);
    updateLocalStateFromRecvBlind(Recv);  // Blind to non-loaded whitespaces
}

function needToLoadWhitespace(){
    return isMissingPrimaryMsgHeap() || isMissingTopMsgHeap() || isMissingBottomMsgHeap();
}

async function tryLoadWhitespaceSingle(){
    if (isMissingPrimaryMsgHeap()){
        await requestMessageNeighbours(-1, "backward");
    } else if (isMissingTopMsgHeap()){
        await requestMessageNeighbours(visibleMsgSegStart, "backward");
    } else if (isMissingBottomMsgHeap()){
        await requestMessageNeighbours(visibleMsgSegEnd, "forward");
    }
}

async function loadWhitespaceMultitry(){
    if (needToLoadWhitespace()){
        cancelMainloopTimeout();
        do {
            try {
                await tryLoadWhitespaceSingle();
                if (debugMode)
                    await sleep(900);
            } catch (e) {
                console.error(e);
                await sleep(1500);
            }
        } while (needToLoadWhitespace());
        setMainloopTimeout();
    }
}

async function updateLocalStateFromRecv(Recv){
    updateLocalStateFromRecvBlind(Recv);
    await loadWhitespaceMultitry();
}

async function safeApiRequestWithLocalStUpdate(type, Sent, errMsg){
    try {
        let Recv = await apiRequest(type, Sent)
        await updateLocalStateFromRecv(Recv);
    } catch(e) {
        console.error(e);
        alert(errMsg);
    }
}

function configureMsgDeletionPopupButtons(){
    document.getElementById("msg-deletion-yes").onclick = function(ev){
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
        let Sent = genSentBase();
        Sent.id = storeHiddenMsgIdForDeletionWin;
        safeApiRequestWithLocalStUpdate("deleteMessage", Sent, pres.chat['failed-delete-message']);
    };

    document.getElementById("msg-deletion-no").onclick = function (ev){
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
    }
}

__mainloopDelayMs = 1000;
async function UPDATE(){
    let Recv = await apiRequest("chatPollEvents", genSentBase());
    await updateLocalStateFromRecv(Recv);
}
__guestMainloopPollerAction = UPDATE;

window.onload = function (){
    console.log("Page was loaded");

    document.body.addEventListener("wheel", function (event) {
        let offset = event.deltaY / 3;
        if (offset < 0){
            bumpedAtBottom = false;
        } else if (offset > 0 && !isMissingBottomMsgHeap() && lowestPoint + offset > chatPadding){
            bumpedAtBottom = true;
        }
        offsetOfAnchor += offset;
        updateOffsets();
        loadWhitespaceMultitry().then(dopDopYesYes);
    });

    document.getElementById("message-input").addEventListener("keyup", function (event) {
        if (event.ctrlKey && event.key === 'Enter'){
            let textarea = document.getElementById("message-input");
            let text = String(textarea.innerText);
            textarea.innerText = "";
            let Sent = genSentBase();
            Sent.content = {};
            Sent.content.text = text;
            safeApiRequestWithLocalStUpdate("sendMessage", Sent, pres.chat['failed-send-message']);
        }
    });

    bumpedAtBottom = (openedchat.selectedMessageId < 0);

    let chatWg = document.getElementById("chat-widget");
    let chatWgDebugLinesFnc = function (){
        let H = chatWg.offsetHeight;
        elSetOffsetInChat(document.getElementById("debug-line-lowest"), -softZoneSz);
        elSetOffsetInChat(document.getElementById("debug-line-highest"), H + softZoneSz);
        elSetOffsetInChat(document.getElementById("debug-line-top-padding"), H - chatPadding);
        elSetOffsetInChat(document.getElementById("debug-line-bottom-padding"), chatPadding)
    };
    if (debugMode){
        window.addEventListener("resize", chatWgDebugLinesFnc);
        chatWgDebugLinesFnc();
    }

    configureMsgDeletionPopupButtons();

    updateLocalStateFromChatUpdRespBlind(initial_chatUpdResp);

    setMainloopTimeout();

    loadWhitespaceMultitry();
}
