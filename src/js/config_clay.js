module.exports = [
  {
    "type": "heading",
    "defaultValue": "Morpheuz Configuration"
  },
  {
    "type": "section",
    "label": "Home Assistant Direct Connection",
    "items": [
      {
        "type": "input",
        "messageKey": "haurl",
        "label": "Home Assistant URL",
        "description": "Indirizzo del server (es. http://192.168.1.x:8123).",
        "attributes": {
          "placeholder": "http://192.168.1.x:8123",
          "type": "text"
        }
      },
      {
        "type": "input",
        "messageKey": "hatoken",
        "label": "Long-Lived Access Token",
        "description": "Generato dal profilo utente in Home Assistant.",
        "attributes": {
          "type": "text"
        }
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];