export type ErrorCode =
  | 'invalid_argument'
  | 'permission_denied'
  | 'not_found'
  | 'rate_limited'
  | 'unsupported'
  | 'cancelled'
  | 'internal';

export interface IpcError {
  code: ErrorCode;
  message: string;
  details?: Record<string, unknown>;
}

export interface IpcRequest<T = unknown> {
  v: number;
  id: string;
  cmd: string;
  args?: T;
}

export interface IpcResponse<T = unknown> {
  v: number;
  id: string;
  ok: boolean;
  data?: T;
  error?: IpcError;
}

export interface SystemInfo {
  os: string;
  version: string;
  arch: string;
  cpuCount: number;
  totalMemoryBytes: number;
  availableMemoryBytes: number;
}

export interface FileFilter {
  name: string;
  extensions: string[];
}

export interface OpenDialogOptions {
  title?: string;
  multiple?: boolean;
  directory?: boolean;
  filters?: FileFilter[];
}

export interface OpenDialogResult {
  canceled: boolean;
  filePaths: string[];
}

export interface SaveDialogOptions {
  title?: string;
  filters?: FileFilter[];
}

export interface SaveDialogResult {
  canceled: boolean;
  filePath: string;
}
