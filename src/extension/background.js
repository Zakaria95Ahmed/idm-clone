/**
 * background.js - Service Worker for IDM Clone browser extension
 * 
 * Monitors HTTP requests to detect downloadable files and video streams.
 * Communicates with the IDM Clone native application via Native Messaging.
 */

const NATIVE_HOST = "com.idmclone.native";

// File extensions to capture
const CAPTURE_EXTENSIONS = new Set([
    'exe', 'zip', 'rar', '7z', 'mp3', 'mp4', 'avi', 'mkv', 'pdf', 'doc',
    'iso', 'torrent', 'flv', 'wmv', 'mov', 'wav', 'aac', 'flac', 'gz',
    'bz2', 'tar', 'xz', 'msi', 'dmg', 'apk', 'deb', 'rpm', 'bin', 'img',
    'cab', 'docx', 'xlsx', 'pptx', 'webm', 'm4v', 'opus', 'ogg', 'wma'
]);

// Content types to capture
const CAPTURE_CONTENT_TYPES = new Set([
    'application/octet-stream', 'application/zip', 'application/x-rar-compressed',
    'application/x-7z-compressed', 'application/pdf', 'application/x-msdownload',
    'audio/mpeg', 'audio/wav', 'audio/flac', 'video/mp4', 'video/x-matroska',
    'video/webm', 'application/x-bittorrent'
]);

// Native messaging port
let nativePort = null;

function connectToNative() {
    try {
        nativePort = chrome.runtime.connectNative(NATIVE_HOST);
        nativePort.onMessage.addListener((msg) => {
            console.log("IDM Clone response:", msg);
        });
        nativePort.onDisconnect.addListener(() => {
            console.log("IDM Clone disconnected");
            nativePort = null;
        });
    } catch (e) {
        console.log("Cannot connect to IDM Clone native app:", e);
    }
}

function sendToIDM(downloadInfo) {
    if (!nativePort) connectToNative();
    
    if (nativePort) {
        nativePort.postMessage(downloadInfo);
    } else {
        // Fallback: store for later
        console.log("IDM Clone not available, queuing download:", downloadInfo.url);
    }
}

// Monitor downloads to intercept them
chrome.downloads.onCreated.addListener((downloadItem) => {
    const url = downloadItem.url;
    const filename = downloadItem.filename || '';
    const ext = filename.split('.').pop().toLowerCase();
    
    if (CAPTURE_EXTENSIONS.has(ext)) {
        // Cancel browser download and send to IDM Clone
        chrome.downloads.cancel(downloadItem.id);
        
        sendToIDM({
            action: "download",
            url: url,
            filename: filename,
            referrer: downloadItem.referrer || '',
            fileSize: downloadItem.totalBytes || -1,
            mimeType: downloadItem.mime || ''
        });
    }
});

// Context menu integration
chrome.runtime.onInstalled.addListener(() => {
    chrome.contextMenus.create({
        id: "download-with-idm",
        title: "Download with IDM Clone",
        contexts: ["link", "image", "video", "audio"]
    });
    
    chrome.contextMenus.create({
        id: "download-all-links",
        title: "Download All Links with IDM Clone",
        contexts: ["page"]
    });
});

chrome.contextMenus.onClicked.addListener((info, tab) => {
    if (info.menuItemId === "download-with-idm") {
        const url = info.linkUrl || info.srcUrl || info.pageUrl;
        sendToIDM({
            action: "download",
            url: url,
            referrer: info.pageUrl || '',
            cookies: ''  // Will be populated by native host
        });
    } else if (info.menuItemId === "download-all-links") {
        // Send message to content script to collect all links
        chrome.tabs.sendMessage(tab.id, { action: "collectLinks" });
    }
});

// Listen for messages from content script
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.action === "downloadWithIDM") {
        sendToIDM({
            action: "download",
            url: message.url,
            filename: message.filename || '',
            referrer: message.referrer || sender.tab?.url || '',
            cookies: message.cookies || ''
        });
        sendResponse({ status: "sent" });
    } else if (message.action === "videoDetected") {
        // Video stream detected by content script
        sendToIDM({
            action: "videoDownload",
            url: message.url,
            quality: message.quality || '',
            format: message.format || '',
            referrer: sender.tab?.url || '',
            title: message.title || ''
        });
    } else if (message.action === "allLinks") {
        // Batch download all links
        for (const link of message.links) {
            sendToIDM({
                action: "download",
                url: link.url,
                filename: link.text || '',
                referrer: sender.tab?.url || ''
            });
        }
    }
    return true;
});

console.log("IDM Clone extension loaded");
