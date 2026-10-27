import type {
  IpcRequest,
  IpcResponse,
  IpcError,
  SystemInfo,
  OpenDialogOptions,
  OpenDialogResult,
  SaveDialogOptions,
  SaveDialogResult,
} from './types';

export * from './types';

declare global {
  interface Window {
    __aura_ipc?: (req: string | IpcRequest) => Promise<string | IpcResponse>;
    __aura_emit?: (event: string, payload: unknown) => void;
  }
}

export class AuraError extends Error {
  public readonly code: string;
  public readonly details?: Record<string, unknown>;

  constructor(error: IpcError) {
    super(error.message);
    this.name = 'AuraError';
    this.code = error.code;
    this.details = error.details;
  }
}

let requestIdCounter = 0;
const eventListeners = new Map<string, Set<(payload: unknown) => void>>();

// Set up global event reception from native backend
if (typeof window !== 'undefined') {
  window.__aura_emit = (event: string, payload: unknown) => {
    const listeners = eventListeners.get(event);
    if (listeners) {
      listeners.forEach((cb) => {
        try {
          cb(payload);
        } catch (err) {
          console.error(`[AuraUI] Error in event listener for '${event}':`, err);
        }
      });
    }
  };
}

/**
 * Invoke a native backend command over the zero-trust in-process IPC channel.
 */
export async function invoke<T = unknown>(
  cmd: string,
  args: Record<string, unknown> = {}
): Promise<T> {
  if (typeof window === 'undefined' || !window.__aura_ipc) {
    throw new AuraError({
      code: 'internal',
      message: 'AuraUI native IPC binding is not available in this environment',
    });
  }

  const id = `req_${++requestIdCounter}_${Date.now().toString(36)}`;
  const request: IpcRequest = {
    v: 1,
    id,
    cmd,
    args,
  };

  const rawResponse = await window.__aura_ipc(JSON.stringify(request));
  const response: IpcResponse<T> =
    typeof rawResponse === 'string' ? JSON.parse(rawResponse) : rawResponse;

  if (!response.ok) {
    throw new AuraError(
      response.error || {
        code: 'internal',
        message: 'Unknown backend error occurred',
      }
    );
  }

  return response.data as T;
}

/**
 * Listen for native events emitted by the C++ backend.
 */
export function on<T = unknown>(
  event: string,
  callback: (data: T) => void
): () => void {
  if (!eventListeners.has(event)) {
    eventListeners.set(event, new Set());
  }

  const set = eventListeners.get(event)!;
  const wrapped = callback as (data: unknown) => void;
  set.add(wrapped);

  return () => {
    set.delete(wrapped);
    if (set.size === 0) {
      eventListeners.delete(event);
    }
  };
}

/**
 * Native window management module
 */
export const windowApi = {
  setTitle: (title: string) => invoke<{ success: boolean }>('window.setTitle', { title }),
  setSize: (width: number, height: number) => invoke<{ success: boolean }>('window.setSize', { width, height }),
  close: () => invoke<{ success: boolean }>('window.close'),
};

/**
 * Native file dialogs module
 */
export const dialog = {
  openFile: (options: OpenDialogOptions = {}) =>
    invoke<OpenDialogResult>('dialog.openFile', options as Record<string, unknown>),
  saveFile: (options: SaveDialogOptions = {}) =>
    invoke<SaveDialogResult>('dialog.saveFile', options as Record<string, unknown>),
};

/**
 * Native system clipboard module
 */
export const clipboard = {
  readText: async () => {
    const res = await invoke<{ text: string }>('clipboard.readText');
    return res.text;
  },
  writeText: (text: string) => invoke<{ success: boolean }>('clipboard.writeText', { text }),
};

/**
 * Native system information module
 */
export const system = {
  getInfo: () => invoke<SystemInfo>('system.getInfo'),
};

/**
 * Native OS shell execution module
 */
export const shell = {
  openExternal: (url: string) => invoke<{ success: boolean }>('shell.openExternal', { url }),
};

/**
 * Sandboxed local filesystem module
 */
export const fs = {
  readFile: async (path: string) => {
    const res = await invoke<{ content: string }>('fs.readFile', { path });
    return res.content;
  },
  writeFile: (path: string, content: string) =>
    invoke<{ bytesWritten: number }>('fs.writeFile', { path, content }),
};

/**
 * Persistent atomic key-value store module
 */
export const store = {
  get: async <T = unknown>(key: string): Promise<T | null> => {
    const res = await invoke<{ found: boolean; value: T | null }>('store.get', { key });
    return res.found ? res.value : null;
  },
  set: (key: string, value: unknown) => invoke<{ success: boolean }>('store.set', { key, value }),
  delete: async (key: string): Promise<boolean> => {
    const res = await invoke<{ deleted: boolean }>('store.delete', { key });
    return res.deleted;
  },
};
