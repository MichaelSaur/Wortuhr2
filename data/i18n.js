const translations = {
    de: {
        pageTitle: "Wortuhr Einstellungen",
        cardTime: "Zeit",
        clockTimeLabel: "Zeit der Uhr",
        currentTimeLabel: "Aktuelle Zeit",
        syncLabel: "Sync",
        apModeNote: "Kein Internet – NTP-Sync nicht möglich. Zeit hier setzen:",
        timezoneLabel: "Zeitzone",
        cardColors: "Farben",
        primaryColor: "Primärfarbe",
        modeLabel: "Modus",
        brightnessLabel: "Helligkeit",
        cardNightMode: "Nachtmodus",
        enableNightMode: "Nachtmodus aktivieren",
        startTimeLabel: "Startzeit",
        endTimeLabel: "Endzeit",
        cardWifi: "WiFi",
        selectSSID: "SSID auswählen",
        passwordLabel: "Passwort",
        passwordPlaceholder: "Passwort eingeben",
        forgetWifi: "WLAN vergessen",
        forgetWifiConfirm: "WLAN-Zugangsdaten wirklich löschen? Die Uhr startet neu und öffnet ein eigenes WLAN namens \"Wortuhr\" zur Neueinrichtung.",
        themeSystem: "System",
        themeLight: "Hell",
        themeDark: "Dunkel",
        loading: "lädt...",
        saveFailed: "Speichern fehlgeschlagen"
    },
    en: {
        pageTitle: "Wortuhr Settings",
        cardTime: "Time",
        clockTimeLabel: "Clock time",
        currentTimeLabel: "Current time",
        syncLabel: "Sync",
        apModeNote: "No internet — NTP sync unavailable. Set the time here:",
        timezoneLabel: "Timezone",
        cardColors: "Colors",
        primaryColor: "Primary color",
        modeLabel: "Mode",
        brightnessLabel: "Brightness",
        cardNightMode: "Night mode",
        enableNightMode: "Enable night mode",
        startTimeLabel: "Start time",
        endTimeLabel: "End time",
        cardWifi: "WiFi",
        selectSSID: "Select SSID",
        passwordLabel: "Password",
        passwordPlaceholder: "Enter password",
        forgetWifi: "Forget WiFi",
        forgetWifiConfirm: "Really delete the saved WiFi credentials? The clock will restart and open its own network named \"Wortuhr\" for reconfiguration.",
        themeSystem: "System",
        themeLight: "Light",
        themeDark: "Dark",
        loading: "loading...",
        saveFailed: "Failed to save data"
    }
};

const locales = { de: "de-DE", en: "en-GB" };

window.currentLang = "de";

function t(key) {
    return (translations[window.currentLang] && translations[window.currentLang][key]) || key;
}

function currentLocale() {
    return locales[window.currentLang] || "de-DE";
}

function applyLanguage(lang) {
    if (!translations[lang]) lang = "de";
    window.currentLang = lang;
    document.documentElement.lang = lang;

    document.querySelectorAll("[data-i18n]").forEach(el => {
        el.textContent = t(el.dataset.i18n);
    });
    document.querySelectorAll("[data-i18n-placeholder]").forEach(el => {
        el.placeholder = t(el.dataset.i18nPlaceholder);
    });
    document.querySelectorAll("[data-i18n-title]").forEach(el => {
        el.title = t(el.dataset.i18nTitle);
    });
    document.querySelectorAll(".langButton").forEach(btn => {
        btn.classList.toggle("btn-primary", btn.dataset.lang === lang);
    });
}
