let LocalHistoryId = 0;

function genSentBase(){
    return {
        'chatUpdReq': {
            'LocalHistoryId': LocalHistoryId,
            'chatId': openedchat.id
        }
    };
}

let members = new Map();
let memberBoxes = new Map();
let myRoleHere = null;  // Dung local state updates should be updated first

let userDeletionWinStoredUId = -1;

function shouldShowDeleteButton(memberSt){
    return userinfo.uid !== memberSt.userId && myRoleHere === userChatRoleAdmin && memberSt.roleHere !== userChatRoleDeleted;
}

function updateBoxWithSt(box, memberSt){
    let ID = memberSt.userId;
    let roleP = box.querySelector(".CM-member-box-role");
    roleP.innerText = memberSt.roleHere;
    box.style.backgroundColor = roleToColor(memberSt.roleHere);
    box.querySelector(".CM-member-box-leave-btn").style.display =
        (shouldShowDeleteButton(memberSt) ? "block" : "none");
}

function convertMemberStToBox(memberSt){
    let ID = memberSt.userId;
    let userProfileURI = "/user/" + memberSt.nickname;

    let box = document.createElement("div");
    box.className = "dynamic-block-list-el CM-member-box";
    box.style.backgroundColor = roleToColor(memberSt.roleHere);

    let inBoxNickname = document.createElement("a");
    box.appendChild(inBoxNickname);
    inBoxNickname.className = "entity-nickname-txt CM-member-box-nickname";
    inBoxNickname.innerText = memberSt.nickname;
    inBoxNickname.href = userProfileURI;

    let inBoxName = document.createElement("a");
    box.appendChild(inBoxName);
    inBoxName.className = "entity-reg-field-txt CM-member-box-name";
    inBoxName.innerText = memberSt.name;
    inBoxName.href = userProfileURI;

    let inBoxUserRoleHere = document.createElement("p");
    box.appendChild(inBoxUserRoleHere);
    inBoxUserRoleHere.className = "entity-reg-field-txt CM-member-box-role";
    inBoxUserRoleHere.innerText = memberSt.roleHere;

    let inBoxLeaveBtn = document.createElement("img");
    box.appendChild(inBoxLeaveBtn);
    inBoxLeaveBtn.className = "CM-member-box-leave-btn";
    inBoxLeaveBtn.src = "/assets/img/delete.svg";
    inBoxLeaveBtn.onclick = function (ev) {
        if (ev.button !== 0)
            return;
        userDeletionWinStoredUId = ID;
        document.getElementById("user-deletion-win-title").innerText =
            pres['chat-members']['reask-kick-user-X'] + " " + memberSt.nickname + "?";
        activatePopupWindowById("user-deletion-win");
    };
    box.querySelector(".CM-member-box-leave-btn").style.display =
        (shouldShowDeleteButton(memberSt) ? "block" : "none");

    return box;
}

function updateLocalStateFromChatUpdResp(chatUpdResp){
    LocalHistoryId = chatUpdResp.HistoryId;
    // If my role is updated, we need to update all the boes of already set users (kick button can appear and disappear)
    let literalMemberList = document.getElementById("CM-list");
    // We ignore messages and everything related to them. Dang, I really should add an argument to disable message lookup here
    for (let memberSt of chatUpdResp.members){
        console.log([memberSt, userinfo.uid, myRoleHere]);
        if (memberSt.userId === userinfo.uid && myRoleHere !== memberSt.roleHere){
            myRoleHere = memberSt.roleHere;
            for (let [id, memberSt] of members){
                let box = memberBoxes.get(id);
                updateBoxWithSt(box, memberSt);
            }
            document.getElementById("CM-btn-add").style.display =
                (memberSt.roleHere === userChatRoleAdmin ? "block" : "none");
            console.log("DEBUG " + (memberSt.roleHere === userChatRoleAdmin ? "block" : "none"));
            break;
        }
    }
    for (let memberSt of chatUpdResp.members){
        let id = memberSt.userId;
        if (members.has(id)){
            updateBoxWithSt(memberBoxes.get(id), memberSt);
        } else {
            if (memberSt.roleHere !== userChatRoleDeleted){
                members.set(id, memberSt);
                let box = convertMemberStToBox(memberSt);
                memberBoxes.set(id, box);
                literalMemberList.appendChild(box);
            }
        }
    }
}

function updateLocalStateFromRecv(Recv){
    updateLocalStateFromChatUpdResp(Recv.chatUpdResp);
}

function configureSummonUserInterface(){
    document.getElementById("user-summoning-yes").onclick = function(ev ){
        if (ev.button !==0)
            return;
        let nickname = String(document.getElementById("summoned-user-nickname").value);
        let isReadOnly = document.getElementById("summoned-user-is-read-only").checked;
        deactivateActivePopup();
        let Sent = genSentBase();
        Sent.nickname = nickname;
        Sent.makeReadOnly = Boolean(isReadOnly);
        apiRequest("addMemberToChat", Sent).
        then((Recv) => {
            updateLocalStateFromRecv(Recv);
        }).catch((e) => {
           console.log(e);
           alert(pres['chat-members']["failed-summon-member"]);
        });
    };

    document.getElementById("user-summoning-no").onclick = function (ev) {
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
    };

    document.getElementById("CM-btn-add").onclick = function(ev) {
        if (ev.button !== 0)
            return;
        document.getElementById("summoned-user-nickname").value = "";
        // read-only flag persists throughout user summoning sessions, and IT IS NOT A BUG
        activatePopupWindowById("user-summoning-win");
    };
}

/* Popup activation button is configured for each box separately */
function configureKickUserInterfaceWinPart(){
    document.getElementById("user-deletion-yes").onclick = function (ev){
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
        if (userDeletionWinStoredUId < 0)
            throw new Error("Karaul");
        let Sent = genSentBase();
        Sent.userId = userDeletionWinStoredUId;
        apiRequest("removeMemberFromChat", Sent).
        then((Recv) => {
            updateLocalStateFromRecv(Recv);
        }).catch((e) => {
            console.log(e);
            alert(pres['chat-members']["failed-kick-member"]);
        });
    }

    document.getElementById("user-deletion-no").onclick = function (ev) {
        if (ev.button !== 0)
            return;
        deactivateActivePopup();
    };
}

__mainloopDelayMS = 5000;
__guestMainloopPollerAction = function (){
    console.log("Hello, world");
    apiRequest("chatPollEvents", genSentBase()).
    then((Recv) => {
        console.log(Recv);
        updateLocalStateFromRecv(Recv);
    });
}

window.onload = function(){
    console.log("Page loaded");
    configureSummonUserInterface();
    configureKickUserInterfaceWinPart();
    updateLocalStateFromChatUpdResp(initial_chatUpdResp);
    mainloopPoller();
}
