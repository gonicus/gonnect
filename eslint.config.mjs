import globals from "globals";

export default [
    {
        files: ["resources/webengine/*.js"],
        languageOptions: {
            ecmaVersion: "latest",
            sourceType: "script",
            globals: {
                ...globals.browser,
                ...globals.worker,
                JitsiMeetJS: "readonly",
                config: "readonly",
                // Placeholders replaced by C++ before the script is injected
                __GONNECT_SESSION_CONFIG__: "readonly",
                __GONNECT_REMOTE_VIDEO_CONFIG__: "readonly",
            },  
        },  
        rules: {
            "no-undef": "error",
        },  
    },
];
