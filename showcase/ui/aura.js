/**
 * AuraUI 2.0 - Vanilla JS Client Library
 * Implements the AuraUI 2 in-process IPC protocol
 */
(function(window) {
    'use strict';

    let requestCounter = 0;
    const eventHandlers = new Map();

    window.__aura_emit = function(event, payload) {
        const handlers = eventHandlers.get(event);
        if (handlers) {
            handlers.forEach(fn => {
                try { fn(payload); } catch (e) { console.error('Aura event error:', e); }
            });
        }
    };

    const Aura = {
        /**
         * Invoke native C++ command
         */
        invoke: async function(cmd, args = {}) {
            if (!window.__aura_ipc) {
                throw new Error("AuraUI native IPC channel not detected.");
            }

            const id = 'req_' + (++requestCounter) + '_' + Date.now();
            const envelope = {
                v: 1,
                id: id,
                cmd: cmd,
                args: args
            };

            const raw = await window.__aura_ipc(JSON.stringify(envelope));
            const resp = (typeof raw === 'string') ? JSON.parse(raw) : raw;

            if (!resp.ok) {
                const err = new Error(resp.error ? resp.error.message : 'Unknown backend error');
                err.code = resp.error ? resp.error.code : 'internal';
                err.details = resp.error ? resp.error.details : null;
                throw err;
            }

            return resp.data;
        },

        on: function(event, callback) {
            if (!eventHandlers.has(event)) {
                eventHandlers.set(event, new Set());
            }
            eventHandlers.get(event).add(callback);
            return function() {
                eventHandlers.get(event).delete(callback);
            };
        },

        window: {
            setTitle: (title) => Aura.invoke('window.setTitle', { title }),
            setSize: (width, height) => Aura.invoke('window.setSize', { width, height }),
            close: () => Aura.invoke('window.close')
        },

        system: {
            getInfo: () => Aura.invoke('system.getInfo')
        },

        clipboard: {
            readText: async () => {
                const res = await Aura.invoke('clipboard.readText');
                return res.text;
            },
            writeText: (text) => Aura.invoke('clipboard.writeText', { text })
        },

        dialog: {
            openFile: (options = {}) => Aura.invoke('dialog.openFile', options),
            saveFile: (options = {}) => Aura.invoke('dialog.saveFile', options)
        },

        store: {
            get: async (key) => {
                const res = await Aura.invoke('store.get', { key });
                return res.found ? res.value : null;
            },
            set: (key, value) => Aura.invoke('store.set', { key, value }),
            delete: async (key) => {
                const res = await Aura.invoke('store.delete', { key });
                return res.deleted;
            }
        }
    };

    window.Aura = Aura;
})(window);
