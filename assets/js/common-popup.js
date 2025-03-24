let activePopupWinId = "";

function activatePopupWindow__(el){
    let veil = document.createElement("div");
    veil.id = "popup-overlay-veil-OBJ"
    veil.className = "popup-overlay-veil";
    veil.style.display = "block";
    document.body.appendChild(veil);
    el.style.display = "block";
}

function activatePopupWindowById(id){
    if (activePopupWinId !== "")
        return;
    /* Lmao, this thing is just... SO unsafe */
    activePopupWinId = id;
    activatePopupWindow__(document.getElementById(id))
}

function deactivateActivePopup(){
    if (activePopupWinId === "")
        return
    document.getElementById("popup-overlay-veil-OBJ").remove();
    document.getElementById(activePopupWinId).style.display = "none";
    activePopupWinId = "";
}
