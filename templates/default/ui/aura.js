/**
 * AuraUI Client Library
 * Seamless bridge between Modern HTML/CSS/JS Frontend and C++ Backend
 */

(function (window) {
    'use strict';

    const eventListeners = new Map();

    const aura = {
        /**
         * Call a C++ function bound with app.bind("name", ...)
         * @param {string} functionName Name of the C++ function
         * @param {any} data JSON-serializable argument (object, array, string, number)
         * @returns {Promise<any>} Response parsed from C++ JSON
         */
        async call(functionName, data = {}) {
            // Wait for WebUI bridge to be ready
            await aura.__ensureReady();

            let targetFn = window[functionName];

            // Normalize route path if not directly matched
            if (typeof targetFn !== 'function' && typeof functionName === 'string') {
                const trimmed = functionName.replace(/^\/+|\/+$/g, '');
                targetFn = window[trimmed] || 
                           window['/' + trimmed] || 
                           window[trimmed.replace(/\//g, '.')] || 
                           window[trimmed.replace(/\//g, '_')];
            }

            if (typeof targetFn !== 'function') {
                throw new Error(`AuraUI: C++ route or function "${functionName}" is not bound in backend.`);
            }

            try {
                const payload = (typeof data === 'object') ? JSON.stringify(data) : String(data);
                const rawResponse = await targetFn(payload);

                if (rawResponse === undefined || rawResponse === null || rawResponse === '') {
                    return null;
                }

                try {
                    return JSON.parse(rawResponse);
                } catch {
                    return rawResponse;
                }
            } catch (err) {
                console.error(`AuraUI call to "${functionName}" failed:`, err);
                throw err;
            }
        },

        /**
         * Listen to events pushed from C++ via app.emit("eventName", data)
         */
        on(eventName, handler) {
            if (!eventListeners.has(eventName)) {
                eventListeners.set(eventName, new Set());
            }
            eventListeners.get(eventName).add(handler);
            return () => aura.off(eventName, handler);
        },

        /**
         * Remove an event listener
         */
        off(eventName, handler) {
            if (eventListeners.has(eventName)) {
                eventListeners.get(eventName).delete(handler);
            }
        },

        /**
         * Internal event dispatcher called by C++ app.emit()
         */
        __dispatch(eventName, payload) {
            if (eventListeners.has(eventName)) {
                for (const handler of eventListeners.get(eventName)) {
                    try {
                        handler(payload);
                    } catch (e) {
                        console.error(`AuraUI error in event listener for "${eventName}":`, e);
                    }
                }
            }
        },

        /**
         * Window management methods
         */
        window: {
            minimize() {
                return aura.call('__aura_win_minimize');
            },
            maximize() {
                return aura.call('__aura_win_maximize');
            },
            close() {
                return aura.call('__aura_win_close');
            }
        },

        /**
         * Native dialogs
         */
        dialog: {
            async open(options = {}) {
                return await aura.call('__aura_dialog_open', options);
            },
            async save(options = {}) {
                return await aura.call('__aura_dialog_save', options);
            },
            async openFolder(options = {}) {
                return await aura.call('__aura_dialog_folder', options);
            }
        },

        /**
         * Native Clipboard (read / write)
         */
        clipboard: {
            async write(text) {
                const res = await aura.call('__aura_clipboard_write', { text: String(text) });
                return res && res.success;
            },
            async read() {
                const res = await aura.call('__aura_clipboard_read');
                return (res && res.text) ? res.text : '';
            }
        },

        /**
         * Native Shell (open URLs in default browser, open files in Explorer)
         */
        shell: {
            async open(target) {
                const res = await aura.call('__aura_shell_open', { target: String(target) });
                return res && res.success;
            }
        },

        /**
         * Native system info (RAM, CPU, OS)
         */
        system: {
            async getInfo() {
                return await aura.call('__aura_sys_info');
            }
        },

        /**
         * Client-Side Single Page App (SPA) Router
         */
        router: {
            _current: null,
            _handlers: new Map(),
            _guards: [],

            get current() {
                return aura.router._current || '/';
            },

            on(path, callback) {
                if (!aura.router._handlers.has(path)) {
                    aura.router._handlers.set(path, new Set());
                }
                aura.router._handlers.get(path).add(callback);
                return () => aura.router._handlers.get(path).delete(callback);
            },

            beforeEach(guardFn) {
                aura.router._guards.push(guardFn);
            },

            async navigate(path, params = {}) {
                if (!path) path = '/';
                if (aura.router._current === path) return;

                const from = aura.router._current;
                let allowed = true;

                for (const guard of aura.router._guards) {
                    await guard(path, from, (proceed = true) => {
                        allowed = proceed;
                    });
                    if (!allowed) return;
                }

                aura.router._current = path;
                try {
                    sessionStorage.setItem('__aura_last_route', path);
                } catch(e) {}

                // 1. Toggle view containers
                const views = document.querySelectorAll('[data-aura-view]');
                let foundMatch = false;

                views.forEach(v => {
                    const viewRoute = v.getAttribute('data-aura-view');
                    if (viewRoute === path) {
                        v.classList.add('active');
                        foundMatch = true;
                    } else {
                        v.classList.remove('active');
                    }
                });

                if (!foundMatch) {
                    const fallback = document.querySelector('[data-aura-view="*"]');
                    if (fallback) fallback.classList.add('active');
                }

                // 2. Toggle active state on route links
                const links = document.querySelectorAll('[data-aura-route]');
                links.forEach(l => {
                    const linkRoute = l.getAttribute('data-aura-route');
                    if (linkRoute === path) {
                        l.classList.add('active');
                    } else {
                        l.classList.remove('active');
                    }
                });

                // 3. Trigger route hooks
                if (aura.router._handlers.has(path)) {
                    for (const cb of aura.router._handlers.get(path)) {
                        try { cb(params); } catch(err) { console.error('Aura Router hook error:', err); }
                    }
                }
            },

            init() {
                let initial = '/';
                try {
                    initial = sessionStorage.getItem('__aura_last_route') || '/';
                } catch(e) {}

                const matchEl = document.querySelector(`[data-aura-view="${initial}"]`);
                if (!matchEl) {
                    const firstView = document.querySelector('[data-aura-view]');
                    if (firstView) initial = firstView.getAttribute('data-aura-view') || '/';
                }

                aura.router.navigate(initial);
            }
        },

        /**
         * Persistent App State Store
         */
        store: {
            get(key, defaultVal = null) {
                try {
                    const item = localStorage.getItem(`aura_${key}`);
                    return item !== null ? JSON.parse(item) : defaultVal;
                } catch {
                    return defaultVal;
                }
            },
            set(key, value) {
                try {
                    localStorage.setItem(`aura_${key}`, JSON.stringify(value));
                    return true;
                } catch {
                    return false;
                }
            },
            remove(key) {
                try {
                    localStorage.removeItem(`aura_${key}`);
                } catch {}
            }
        },

        __ensureReady() {
            return new Promise((resolve) => {
                if (window.webui && window.webui.isConnected) {
                    resolve();
                } else {
                    const checkInterval = setInterval(() => {
                        if (typeof window.webui !== 'undefined') {
                            clearInterval(checkInterval);
                            resolve();
                        }
                    }, 20);
                    // Fallback timeout after 1.5 seconds
                    setTimeout(() => {
                        clearInterval(checkInterval);
                        resolve();
                    }, 1500);
                }
            });
        }
    };

    window.aura = aura;

    // Attach custom window title bar button handlers automatically if elements exist
    document.addEventListener('DOMContentLoaded', () => {
        const minBtn = document.getElementById('aura-win-min');
        const maxBtn = document.getElementById('aura-win-max');
        const closeBtn = document.getElementById('aura-win-close');

        if (minBtn) minBtn.addEventListener('click', () => aura.window.minimize());
        if (maxBtn) maxBtn.addEventListener('click', () => aura.window.maximize());
        if (closeBtn) closeBtn.addEventListener('click', () => aura.window.close());

        // Declarative Routing link clicks
        document.body.addEventListener('click', (e) => {
            const routeEl = e.target.closest('[data-aura-route]');
            if (routeEl) {
                e.preventDefault();
                const route = routeEl.getAttribute('data-aura-route');
                aura.router.navigate(route);
                return;
            }

            const openEl = e.target.closest('[data-aura-open]');
            if (openEl) {
                e.preventDefault();
                aura.shell.open(openEl.getAttribute('data-aura-open'));
                return;
            }

            const copyEl = e.target.closest('[data-aura-copy]');
            if (copyEl) {
                const target = copyEl.getAttribute('data-aura-copy');
                let text = target;
                if (target.startsWith('#') || target.startsWith('.')) {
                    const el = document.querySelector(target);
                    if (el) text = el.innerText || el.value;
                }
                aura.clipboard.write(text).then(() => {
                    const orig = copyEl.textContent;
                    copyEl.textContent = 'Copied!';
                    setTimeout(() => copyEl.textContent = orig, 1500);
                });
            }
        });

        // Initialize Router if views exist on the page
        if (document.querySelector('[data-aura-view]')) {
            aura.router.init();
        }
    });

})(window);
