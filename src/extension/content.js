/**
 * content.js - Content script for IDM Clone browser integration
 * 
 * Injected into all web pages to:
 * 1. Detect video/audio elements
 * 2. Show "Download with IDM" button on media players
 * 3. Collect all links for batch download
 */

// Listen for messages from background script
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.action === "collectLinks") {
        const links = [];
        document.querySelectorAll('a[href]').forEach(a => {
            const url = a.href;
            if (url && (url.startsWith('http://') || url.startsWith('https://'))) {
                links.push({
                    url: url,
                    text: a.textContent.trim().substring(0, 100)
                });
            }
        });
        
        chrome.runtime.sendMessage({
            action: "allLinks",
            links: links
        });
        
        sendResponse({ count: links.length });
    }
    return true;
});

// Monitor for download links and add IDM overlay
function addDownloadOverlay(element, url) {
    if (element.dataset.idmOverlay) return;
    element.dataset.idmOverlay = "true";
    
    const overlay = document.createElement('div');
    overlay.className = 'idm-download-overlay';
    overlay.innerHTML = '&#x2B07; Download with IDM';
    overlay.style.cssText = `
        position: absolute;
        top: 5px;
        right: 5px;
        background: rgba(0, 120, 215, 0.9);
        color: white;
        padding: 6px 12px;
        border-radius: 4px;
        font-size: 12px;
        font-family: Segoe UI, sans-serif;
        cursor: pointer;
        z-index: 99999;
        box-shadow: 0 2px 6px rgba(0,0,0,0.3);
        transition: background 0.2s;
    `;
    
    overlay.addEventListener('mouseenter', () => {
        overlay.style.background = 'rgba(0, 100, 200, 0.95)';
    });
    
    overlay.addEventListener('mouseleave', () => {
        overlay.style.background = 'rgba(0, 120, 215, 0.9)';
    });
    
    overlay.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        
        chrome.runtime.sendMessage({
            action: "downloadWithIDM",
            url: url,
            referrer: window.location.href
        });
    });
    
    element.style.position = element.style.position || 'relative';
    element.appendChild(overlay);
}

// Scan for video elements periodically
const videoObserver = new MutationObserver(() => {
    document.querySelectorAll('video[src], video source[src]').forEach(el => {
        const video = el.tagName === 'VIDEO' ? el : el.closest('video');
        const src = el.src || el.getAttribute('src');
        if (src && video) {
            addDownloadOverlay(video.parentElement || video, src);
        }
    });
});

videoObserver.observe(document.body || document.documentElement, {
    childList: true,
    subtree: true
});
