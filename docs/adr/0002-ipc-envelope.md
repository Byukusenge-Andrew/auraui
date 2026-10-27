# ADR-002: Unified IPC Envelope and Dispatcher

## Status
Accepted

## Context
In AuraUI v1, each bound C++ command was directly registered as an individual global JavaScript function (or window method). Built-in handlers bypassed middleware and routing logic, and error propagation was inconsistent.

## Decision
All IPC communication between the web frontend and C++ backend MUST pass through a single in-process bridge:
`__aura_ipc(envelopeString)`

### Envelope Specification
- **Request Envelope**:
  ```json
  {
    "v": 1,
    "id": "req_01h7x9...",
    "cmd": "system.info",
    "args": {}
  }
  ```
- **Success Response**:
  ```json
  {
    "v": 1,
    "id": "req_01h7x9...",
    "ok": true,
    "data": { ... }
  }
  ```
- **Error Response**:
  ```json
  {
    "v": 1,
    "id": "req_01h7x9...",
    "ok": false,
    "error": {
      "code": "permission_denied",
      "message": "Capability clipboard.read is not granted"
    }
  }
  ```

### Constraints
1. Every command is handled by a central `Dispatcher` choke point.
2. The envelope enforces strict limits: max 1 MiB payload, max JSON depth 32, max ID length 64 bytes.
3. Serialization uses `json::error_handler_t::replace` so invalid UTF-8 strings never throw exceptions.

## Consequences
- Guaranteed auditability and uniform middleware enforcement for all requests.
- Attackers cannot bypass capabilities by calling internal binding functions.
