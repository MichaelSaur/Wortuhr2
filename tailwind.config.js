/** Dev-only Tailwind config to regenerate data/full.min.css. Not shipped to the ESP32. */
module.exports = {
  content: ["./data/wortuhr.html", "./data/index.js", "./data/i18n.js"],
  darkMode: "media",
  theme: {
    extend: {},
  },
  plugins: [require("daisyui")],
};
