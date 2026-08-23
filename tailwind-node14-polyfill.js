// Dev-only shim so the Tailwind/daisyUI build works on the locally installed Node 14,
// which predates the Object.hasOwn global (added in Node 16.9). Not shipped to the ESP32.
if (typeof Object.hasOwn !== "function") {
  Object.hasOwn = function (obj, prop) {
    return Object.prototype.hasOwnProperty.call(obj, prop);
  };
}
