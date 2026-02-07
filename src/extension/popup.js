/**
 * popup.js - Extension popup UI logic
 */

document.addEventListener('DOMContentLoaded', () => {
    // Add URL button
    document.getElementById('addUrlBtn').addEventListener('click', () => {
        const url = prompt('Enter URL to download:');
        if (url) {
            chrome.runtime.sendMessage({
                action: "downloadWithIDM",
                url: url,
                referrer: ''
            });
            window.close();
        }
    });
    
    // Open IDM button
    document.getElementById('openIdmBtn').addEventListener('click', () => {
        chrome.runtime.sendNativeMessage("com.idmclone.native", 
            { action: "open" }, 
            (response) => {
                console.log("Open IDM response:", response);
            }
        );
    });
});
