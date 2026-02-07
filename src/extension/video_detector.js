/**
 * video_detector.js - Video stream detection for IDM Clone
 * 
 * Detects video streams from popular sites and adaptive streaming.
 */

(function() {
    'use strict';
    
    // Intercept XHR to detect video manifests
    const originalXHR = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function(method, url) {
        if (typeof url === 'string') {
            // Detect HLS manifests
            if (url.includes('.m3u8')) {
                notifyVideoDetected(url, 'HLS', 'stream');
            }
            // Detect DASH manifests
            if (url.includes('.mpd')) {
                notifyVideoDetected(url, 'DASH', 'stream');
            }
            // Detect common video CDN patterns
            if (url.match(/\.(mp4|webm|flv)\?/i) || 
                url.match(/googlevideo\.com/i) ||
                url.match(/fbcdn.*video/i)) {
                notifyVideoDetected(url, 'Direct', getQualityFromUrl(url));
            }
        }
        return originalXHR.apply(this, arguments);
    };
    
    // Intercept fetch for same purpose
    const originalFetch = window.fetch;
    window.fetch = function(input) {
        const url = typeof input === 'string' ? input : input?.url;
        if (url) {
            if (url.includes('.m3u8')) notifyVideoDetected(url, 'HLS', 'stream');
            if (url.includes('.mpd')) notifyVideoDetected(url, 'DASH', 'stream');
        }
        return originalFetch.apply(this, arguments);
    };
    
    function notifyVideoDetected(url, format, quality) {
        try {
            chrome.runtime.sendMessage({
                action: "videoDetected",
                url: url,
                format: format,
                quality: quality,
                title: document.title,
                pageUrl: window.location.href
            });
        } catch (e) {
            // Extension context may not be available
        }
    }
    
    function getQualityFromUrl(url) {
        const match = url.match(/(\d{3,4})p/);
        return match ? match[1] + 'p' : 'unknown';
    }
})();
