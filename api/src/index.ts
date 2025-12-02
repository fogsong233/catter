import * as capi from "catter-c";

export function print(content: string): void {
  capi.stdout_print(content);
}
