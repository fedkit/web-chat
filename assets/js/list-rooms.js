let LocalHistoryId = 0;

function genSentBase(){
    return {
        'chatListUpdReq': {
            'LocalHistoryId': LocalHistoryId
        }
    };
}

let myChats = new Map();
let chatBoxes = new Map();

/* Generate text that is displayed on the right side of chat intro box */
function youAreXHere(myRoleHere){
    // todo: TRANSLATE IT
    return pres['list-rooms']['you-are-X-here'][0] + " " + myRoleHere + " " + pres['list-rooms']['you-are-X-here'][1];
}


let chatRenunciationWinStoredId = -1;

function shouldShowDeleteButton(myMembershipSt){
    return myMembershipSt.myRoleHere === userChatRoleDeleted;
}

/* Updating chat html box after myMembershipSt in it was updated */
function updateBoxWithNewSt(box, myMembershipSt){
    let ID = myMembershipSt.chatId;
    let roleP = box.querySelector(".CL-my-chat-box-my-role");
    roleP.innerText = youAreXHere(myMembershipSt.myRoleHere);
    box.style.backgroundColor = roleToColor(myMembershipSt.myRoleHere);
    box.querySelector(".CL-my-chat-box-leave-btn").style.display =
        (shouldShowDeleteButton(myMembershipSt) ? "none" : "block");
}

function convertMyMembershipStToBox(myMembershipSt){
    let chatURI = "/chat/" + myMembershipSt.chatNickname;
    let ID = myMembershipSt.chatId;

    let box = document.createElement("div");
    box.className = "dynamic-block-list-el CL-my-chat-box";
    box.style.backgroundColor = roleToColor(myMembershipSt.myRoleHere);

    let inBoxNickname = document.createElement("a");
    box.appendChild(inBoxNickname);
    inBoxNickname.className = "entity-nickname-txt CL-my-chat-box-nickname";
    inBoxNickname.innerText = myMembershipSt.chatNickname;
    inBoxNickname.href = chatURI;

    let inBoxName = document.createElement("a");
    box.appendChild(inBoxName);
    inBoxName.className = "entity-reg-field-txt CL-my-chat-box-name";
    inBoxName.innerText = myMembershipSt.chatName;
    inBoxName.href = chatURI;

    let inBoxMyRoleHere = document.createElement("p");
    box.appendChild(inBoxMyRoleHere);
    inBoxMyRoleHere.className = "entity-reg-field-txt CL-my-chat-box-my-role";
    inBoxMyRoleHere.innerText = youAreXHere(myMembershipSt.myRoleHere);

    let inBoxLeaveBtn = document.createElement("img");
    box.appendChild(inBoxLeaveBtn);
    inBoxLeaveBtn.className = "CL-my-chat-box-leave-btn";
    inBoxLeaveBtn.src = "/assets/img/delete.svg";
    inBoxLeaveBtn.onclick = function (ev) {
        if (ev.button !== 0)
            return;
        chatRenunciationWinStoredId = ID;
        document.getElementById("chat-renunciation-win-title").innerText =
            pres['list-rooms']['reask-leave-chat-X'] + " " + myMembershipSt.chatNickname + "?";
        activatePopupWindowById("chat-renunciation-win");
    };
    box.querySelector(".CL-my-chat-box-leave-btn").style.display =
        (shouldShowDeleteButton(myMembershipSt) ? "none" : "block");
    return box;
}

function updateLocalStateFromChatListUpdResp(chatListUpdResp){
    LocalHistoryId = chatListUpdResp.HistoryId;

    let literalChatList = document.getElementById("CL-dblec");

    for (let myMembershipSt of chatListUpdResp.myChats){
        let chatId = myMembershipSt.chatId;
        console.log(myMembershipSt);
        if (myChats.has(chatId)){
            myChats.set(chatId, myMembershipSt);
            updateBoxWithNewSt(chatBoxes.get(chatId), myMembershipSt);
        } else {
            if (myMembershipSt.myRoleHere === userChatRoleDeleted)
                continue;
            myChats.set(chatId, myMembershipSt);
            let box = convertMyMembershipStToBox(myMembershipSt)
            chatBoxes.set(chatId, box);
            literalChatList.appendChild(box);
        }
    }
}

/* Use it ONLY if `Recv` reported success */
function updateLocalStateFromRecv(Recv){
    updateLocalStateFromChatListUpdResp(Recv.chatListUpdResp);
}

function configureChatCreationInterface(){
    document.getElementById("chat-creation-win-yes").onclick = function (ev) {
        if (ev.button !== 0)
            return;
        let chatNicknameInput = document.getElementById("chat-nickname-input");
        let chatNameInput = document.getElementById("chat-name-input");
        let nickname = String(chatNicknameInput.value);
        let name = String(chatNameInput.value);
        deactivateActivePopup();
        let Sent = genSentBase();
        Sent.content = {};
        Sent.content.nickname = nickname;
        Sent.content.name = name;
        apiRequest("createChat", Sent
        ).then((Recv) => {
            updateLocalStateFromRecv(Recv);
        }).catch((e) => {
            alert(pres['list-rooms']["failed-create-chat"]);
            console.log(e);
        });
    };

    document.getElementById("chat-creation-win-no").onclick = function (ev) {
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
    }

    document.getElementById("CL-bacbe").onclick = function (ev){
        if (ev.button !== 0)
            return;
        let chatNicknameInput = document.getElementById("chat-nickname-input");
        let chatNameInput = document.getElementById("chat-name-input");
        chatNicknameInput.value = "";
        chatNameInput.value = "";
        activatePopupWindowById("chat-creation-win");
    };
}

function configureChatRenunciationInterfaceWinPart(){
    document.getElementById("chat-renunciation-win-yes").onclick = function (ev){
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
        if (chatRenunciationWinStoredId < 0)
            throw new Error("chatRenunciationWinStoredId < 0");
        let chatId = chatRenunciationWinStoredId;
        let Sent = genSentBase();
        Sent.chatId = chatId;
        apiRequest("leaveChat", Sent
        ).then((Recv) => {
            updateLocalStateFromRecv(Recv);
        }).catch((e) => {
            alert(pres['list-rooms']["failed-create-chat"]);
            console.log(e);
        });
    }

    document.getElementById("chat-renunciation-win-no").onclick = function(ev) {
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
    }
}

__mainloopDelayMs = 3000;
__guestMainloopPollerAction = function(){
    let Sent = genSentBase();
    apiRequest("chatListPollEvents", Sent
    ).then((Recv) => {
        console.log("Got a response");
        console.log(Recv);
        updateLocalStateFromRecv(Recv);
    });
}

window.onload = function () {
    console.log("Loading complete");
    updateLocalStateFromChatListUpdResp(initial_chatListUpdResp);
    configureChatCreationInterface();
    configureChatRenunciationInterfaceWinPart();
    mainloopPoller();
};
