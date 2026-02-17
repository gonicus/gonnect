# [2.1.0-beta.3](https://github.com/gonicus/gonnect/compare/v2.1.0-beta.2...v2.1.0-beta.3) (2026-02-13)


### Bug Fixes

* always parse UTC offset / timezone ([#329](https://github.com/gonicus/gonnect/issues/329)) ([c74e7d5](https://github.com/gonicus/gonnect/commit/c74e7d53238669fbb3f46c18fb99267ca7d8e315))
* avoid local time conversion for unknown timezones ([#333](https://github.com/gonicus/gonnect/issues/333)) ([bb45139](https://github.com/gonicus/gonnect/commit/bb45139b1debf06ffc7ca689a8afea2097e1f7fb))
* remove custom libusb building ([658daa1](https://github.com/gonicus/gonnect/commit/658daa1601ff10be0b4959c7399e074b3b9de601))


### Features

* blocking address sources ([#325](https://github.com/gonicus/gonnect/issues/325)) ([ac61c48](https://github.com/gonicus/gonnect/commit/ac61c4840d2321cc1a8f5105ce9e01df303bd1d8)), closes [#312](https://github.com/gonicus/gonnect/issues/312)
* configure jitsi MCU via config ([#337](https://github.com/gonicus/gonnect/issues/337)) ([522de41](https://github.com/gonicus/gonnect/commit/522de41809fca9ad74d78eadf87331a617933e50))
* retrieve and show phone numbers for joining jitsi conference ([#334](https://github.com/gonicus/gonnect/issues/334)) ([ec2804c](https://github.com/gonicus/gonnect/commit/ec2804c01736e9b82e621626a2bfc120d9ec5f4f))
* **ui:** show red dot on active main tab bar button ([#326](https://github.com/gonicus/gonnect/issues/326)) ([676f283](https://github.com/gonicus/gonnect/commit/676f2836aeedb7f2c34a82a82f280943b2fd6073))

# [2.1.0-beta.1](https://github.com/gonicus/gonnect/compare/v2.0.8...v2.1.0-beta.1) (2026-02-09)


### Bug Fixes

* add missing export for CSV addressbook ([3fd2e92](https://github.com/gonicus/gonnect/commit/3fd2e92f61b77d4acd018521a12c2820ac050e89))
* default setting for mute system forwarding ([#293](https://github.com/gonicus/gonnect/issues/293)) ([4a90bd5](https://github.com/gonicus/gonnect/commit/4a90bd5656590f70c106badd78946137c97ac9c0))
* **deps:** update EDS to 3.59.1 ([#285](https://github.com/gonicus/gonnect/issues/285)) ([8e3297b](https://github.com/gonicus/gonnect/commit/8e3297b0eb40988bc0bd8d54f7aeed0d765662bb))
* first aid menu improvements ([#283](https://github.com/gonicus/gonnect/issues/283)) ([4680755](https://github.com/gonicus/gonnect/commit/46807554fe6aeb3535d306900c51acf4bb8d7242))
* init Credentials class in flatpak if no SecretPortal ([#322](https://github.com/gonicus/gonnect/issues/322)) ([cb11ef2](https://github.com/gonicus/gonnect/commit/cb11ef20c94a18e4a1c2a178afa79d009f17f330))
* limit active ui edit mode dialogs to one & close on exiting edit mode ([#309](https://github.com/gonicus/gonnect/issues/309)) ([f2904ad](https://github.com/gonicus/gonnect/commit/f2904ad673adba9f4a6694907df244084ce48c9c))
* local shortcuts and help dialog ([#230](https://github.com/gonicus/gonnect/issues/230)) ([55e2133](https://github.com/gonicus/gonnect/commit/55e213300b94cde1a222c8174049661b7dad11ff))
* optimize search for pure numbers ([0201a25](https://github.com/gonicus/gonnect/commit/0201a259bbcbfe2e38ac2034b9c76644d035879f))
* platform updates ([#259](https://github.com/gonicus/gonnect/issues/259)) ([84d515e](https://github.com/gonicus/gonnect/commit/84d515e61d2467ff2b8f3b450c3591e110da09b0))
* prevent entering of invalid DTMF chars to prevent crash ([#287](https://github.com/gonicus/gonnect/issues/287)) ([42cb3df](https://github.com/gonicus/gonnect/commit/42cb3dfd548c8a607db20c3f473f62558e051132))
* prevent hold of ongoing call on incoming when busyOnBusy ([#297](https://github.com/gonicus/gonnect/issues/297)) ([e061657](https://github.com/gonicus/gonnect/commit/e0616576501dfe01cc961345be2505b4674b9b22))
* **ui:** copy full url to clipboard not just room name (DateEventsList) ([#218](https://github.com/gonicus/gonnect/issues/218)) ([9452bfa](https://github.com/gonicus/gonnect/commit/9452bfada7177d4842fd8afc40f3470a48654f21))
* **ui:** do not disable history items in search list (if number) ([#314](https://github.com/gonicus/gonnect/issues/314)) ([c44b1b2](https://github.com/gonicus/gonnect/commit/c44b1b20bc56d88599c6fd05cf8df901b294b0fe)), closes [#311](https://github.com/gonicus/gonnect/issues/311)
* **ui:** fixed selection behaviour in call list ([#271](https://github.com/gonicus/gonnect/issues/271)) ([734c9d8](https://github.com/gonicus/gonnect/commit/734c9d84677facc8d1dc3d345ba1fb3d0008dc66))
* **ui:** jitsi meet chat now cleared when leaving room ([#239](https://github.com/gonicus/gonnect/issues/239)) ([50c6bc5](https://github.com/gonicus/gonnect/commit/50c6bc51d2e700c9f5c0369d0aa13239642fa00b))
* **ui:** loading of own avatar ([#290](https://github.com/gonicus/gonnect/issues/290)) ([f11c4bc](https://github.com/gonicus/gonnect/commit/f11c4bc0817e06c34cdbbc75b044bf8d0c799346)), closes [#276](https://github.com/gonicus/gonnect/issues/276)
* **ui:** next button in template wizard never enabled ([c11f4ed](https://github.com/gonicus/gonnect/commit/c11f4ed24cacc201a58c8847971eedf3cf6ae0fc))
* **ui:** next button in template wizard never enabled ([#291](https://github.com/gonicus/gonnect/issues/291)) ([3107665](https://github.com/gonicus/gonnect/commit/310766560bd0fa8f495524e63261f3bab85d2955))
* unhold other call when starting conference ([#286](https://github.com/gonicus/gonnect/issues/286)) ([33c43ec](https://github.com/gonicus/gonnect/commit/33c43ec6f07a9b76e4e15e33ccbeafb8fa41d49a))


### Features

* add inhibit to windows ([#280](https://github.com/gonicus/gonnect/issues/280)) ([6e67f93](https://github.com/gonicus/gonnect/commit/6e67f938c830738767d5f691084a4e0b2bbd0f3d))
* allow pre-processing of search strings ([#274](https://github.com/gonicus/gonnect/issues/274)) ([ac104e4](https://github.com/gonicus/gonnect/commit/ac104e40c8b30d4a0fba747e400c2d4c2044e768))
* calendar expansion ([#250](https://github.com/gonicus/gonnect/issues/250)) ([9cfdb2d](https://github.com/gonicus/gonnect/commit/9cfdb2d9d52b1c85e846c1f1c55b769679f198e2))
* do not send notifications for conferences/appointments that have been joined early ([#298](https://github.com/gonicus/gonnect/issues/298)) ([91ce62e](https://github.com/gonicus/gonnect/commit/91ce62e9613c5675d2d26fc5eaebc6afc9245f11))
* dynamic widget pages/dashboards ([#251](https://github.com/gonicus/gonnect/issues/251)) ([829952b](https://github.com/gonicus/gonnect/commit/829952b9a0389de1f3a6227f9d7aa76770fce430))
* improved handling of full- and multi-day conferences/appointments ([#302](https://github.com/gonicus/gonnect/issues/302)) ([e55f897](https://github.com/gonicus/gonnect/commit/e55f89735e9e1a0e2a77f8827cab7af34c2c8022))
* introduce custom widget settings and a new web display widget ([#284](https://github.com/gonicus/gonnect/issues/284)) ([30055e5](https://github.com/gonicus/gonnect/commit/30055e5edb007a7463fb0fc628519e1cb74a5ab2))
* packaging tuner ([#261](https://github.com/gonicus/gonnect/issues/261)) ([6bc1792](https://github.com/gonicus/gonnect/commit/6bc1792455d8c67524e99259d953536cc904a219))
* support more platforms ([#233](https://github.com/gonicus/gonnect/issues/233)) ([19fff59](https://github.com/gonicus/gonnect/commit/19fff592caead967c73517deed84d01d4dfbe690))
* toggle whiteboard in jitsi meet ([#235](https://github.com/gonicus/gonnect/issues/235)) ([23df6a2](https://github.com/gonicus/gonnect/commit/23df6a2a11fa3f1eff1316ddd1116ec5e6a8ccf2))
* **ui:** automatically select first search item in global search ([#292](https://github.com/gonicus/gonnect/issues/292)) ([117945a](https://github.com/gonicus/gonnect/commit/117945a6dbfb5b398459117ec87d03c05d571e0d))

## [2.0.9](https://github.com/gonicus/gonnect/compare/v2.0.8...v2.0.9) (2026-01-21)


### Bug Fixes

* **ui:** next button in template wizard never enabled ([c11f4ed](https://github.com/gonicus/gonnect/commit/c11f4ed24cacc201a58c8847971eedf3cf6ae0fc))

## [2.0.8](https://github.com/gonicus/gonnect/compare/v2.0.7...v2.0.8) (2025-12-11)


### Bug Fixes

* allow enabling GPU in chromium ([8decb97](https://github.com/gonicus/gonnect/commit/8decb974a7859d7d9cc731ec2fe56bc05ab8b45e))
* disable GPU for Qt 6.9 builds ([a7c01ae](https://github.com/gonicus/gonnect/commit/a7c01ae751e429baa17302aad531bcd4c0a7da04))
* don't bail out if there is no secret portal ([0bd1dc4](https://github.com/gonicus/gonnect/commit/0bd1dc41c2cba16e61b15c3aab8dd385e1acb54b))

## [2.0.7](https://github.com/gonicus/gonnect/compare/v2.0.6...v2.0.7) (2025-11-07)

### Bug Fixes

* add template for STARFACE PBX
* add platform abstraction layer
  * add dummy platform
  * use libnotify instead of portal
* forward initial Jitsi mute to headset
* prevent changing of camera in confernece to cause inifinite loop
* bump Qt to 6.9.3
* do not hold other calls on new incoming call when ringing
* added mutex for addressbook operations
* display past recurrent event instances that have been updated
* enable buddy subscription for eds/akonadi sip uris
* async component retrieval (calendar) fix freeze on startup
* adjust path for eds avatar images
* make conference mute button work again
* fixed screen share button on conference page
* fixed hold/unhold handling conflicts
* show correct page on call/conference start/end
* fixes for too narrow screen
* show (correct) own display name in chats
* fixed hold/unhold handling conflicts

## [2.0.6](https://github.com/gonicus/gonnect/compare/v2.0.5...v2.0.6) (2025-10-07)


### Bug Fixes

* allow sending of asterisk via dtmf  ([#201](https://github.com/gonicus/gonnect/issues/201)) ([41c377b](https://github.com/gonicus/gonnect/commit/41c377bd4d18fc0c74e603e5b5557ee5979621a6)), closes [#197](https://github.com/gonicus/gonnect/issues/197)

## [2.0.5](https://github.com/gonicus/gonnect/compare/v2.0.4...v2.0.5) (2025-09-09)


### Bug Fixes

* do not abort on minor SQL migration error ([#164](https://github.com/gonicus/gonnect/issues/164)) ([33a9e2b](https://github.com/gonicus/gonnect/commit/33a9e2b3a52344f2b5f2a33c78dcf66eaf62a1d2))
* fix Jitsi Meet crash when upgrading SIP call with screen share ([#162](https://github.com/gonicus/gonnect/issues/162)) ([ba4ca9f](https://github.com/gonicus/gonnect/commit/ba4ca9f8f8e256f8bdda072c8fa40b267a32f8fd))
* mute behaviour in jitsi rooms ([12def7b](https://github.com/gonicus/gonnect/commit/12def7bea63a58cde78f51c01bb9447b02479a9a))
* revert evolution-data-server back to 3.56.0 ([2f9e9be](https://github.com/gonicus/gonnect/commit/2f9e9be7b2de99572f714bc6311875db2ab4b138))
* show password dialog again ([8164305](https://github.com/gonicus/gonnect/commit/816430571e1082c2a066afeced316998ae0c26bc))
* **ui:** do not show multiple password dialogs ([#165](https://github.com/gonicus/gonnect/issues/165)) ([bb0020e](https://github.com/gonicus/gonnect/commit/bb0020eeb64ad52fb4097f6a45c31fdb87930866))
* use short month names for display headsets ([4449ac0](https://github.com/gonicus/gonnect/commit/4449ac07d891096e9f27445e477ae165647f71da))


### Reverts

* Revert "fix: revert evolution-data-server back to 3.56.0" ([02ce3e9](https://github.com/gonicus/gonnect/commit/02ce3e9c3bc95cbbbe3a0b558f52345432352f01))

## [2.0.4](https://github.com/gonicus/gonnect/compare/v2.0.3...v2.0.4) (2025-09-02)


### Bug Fixes

* autoremove date event notifications ([795f4b0](https://github.com/gonicus/gonnect/commit/795f4b0a469abff6a446700f347c39a114ba1382))
* avoid parallel calls for date event queue processor ([eeabda9](https://github.com/gonicus/gonnect/commit/eeabda90704556fc7e68e4d4e986d5c777223ce0))
* avoid parallel calls for queue processor ([63f2e7b](https://github.com/gonicus/gonnect/commit/63f2e7bdd51f9cdffbefe4fca4792193c0ebd5c4))
* honour tray icon activation reasons ([#156](https://github.com/gonicus/gonnect/issues/156)) ([2eb3378](https://github.com/gonicus/gonnect/commit/2eb3378e347520831a1abc823d7272c4f47233cc))
* ignore timer for unvoluntary hook on via headset ([#138](https://github.com/gonicus/gonnect/issues/138)) ([9e3208c](https://github.com/gonicus/gonnect/commit/9e3208c84f1e35470831acef87308b447610be25))
* properly display event status, fix event notifications (calendar) ([ff7a0cc](https://github.com/gonicus/gonnect/commit/ff7a0cc8ba42e80103267f1332dbc4bc570e7218))
* re-trigger event notifications for updated events ([#154](https://github.com/gonicus/gonnect/issues/154)) ([b1e0f34](https://github.com/gonicus/gonnect/commit/b1e0f34163be2b653b9ab437ac9c171449dbbba1))
* regonize DateEvent as in the past ([ec4e2fb](https://github.com/gonicus/gonnect/commit/ec4e2fb573f1f549e6b8125c4dcbc5cbc8ab66c9))
* revert evolution-data-server back to 3.56.0 ([409b830](https://github.com/gonicus/gonnect/commit/409b8306be09832ef1c6ec092e30408a8b84b771))
* show notification while in conference ([2fdfe7a](https://github.com/gonicus/gonnect/commit/2fdfe7a3eb4564c088186f82d9fca516e9f8bf6c))
* **ui:** change to conference page on video upgrade ([#159](https://github.com/gonicus/gonnect/issues/159)) ([6dff6ab](https://github.com/gonicus/gonnect/commit/6dff6ab59053b93a179dcb9065e6052f3187d20f))
* **ui:** larger font for chat message info label ([0dc4793](https://github.com/gonicus/gonnect/commit/0dc4793498b68f37d58366d1ca07c7bdc4579320))
* **ui:** show correct page after terminating call ([#146](https://github.com/gonicus/gonnect/issues/146)) ([c837c92](https://github.com/gonicus/gonnect/commit/c837c92c507a307e2666f181de29115c15aad425))
* use placeholder identity that looks like a placeholder ([19dd5bb](https://github.com/gonicus/gonnect/commit/19dd5bbc598056ac272f9bd213a66fc0d9523102))


### Reverts

* Revert "fix: revert evolution-data-server back to 3.56.0" ([8f8a19d](https://github.com/gonicus/gonnect/commit/8f8a19d2484afe432507eee3f7f895ba8514fd05))

# [2.0.3](https://github.com/gonicus/gonnect/compare/v2.0.2...v2.1.3) (2025-08-18)


### Bug Fixes

* read settings value for dark tray icon correctly ([#141](https://github.com/gonicus/gonnect/issues/141)) ([a5c044c](https://github.com/gonicus/gonnect/commit/a5c044cf0c73470f6600c04853441a2fb8d2a997))
* revert evolution-data-server back to 3.56.0 ([bf15fe4](https://github.com/gonicus/gonnect/commit/bf15fe4914ecb474dd4376c3249223c3dc2911ee))
* segfault happend in certain hold states ([#142](https://github.com/gonicus/gonnect/issues/142)) ([8199174](https://github.com/gonicus/gonnect/commit/81991741d8d906680dc5842c697c036d6cf71c11))
* **ui:** better color for button bar labels in dark mode ([#143](https://github.com/gonicus/gonnect/issues/143)) ([420db09](https://github.com/gonicus/gonnect/commit/420db09c57d39fac40f3ae9534be338465b2ec28))


### Features

* emergency numbers are now configurable ([#140](https://github.com/gonicus/gonnect/issues/140)) ([5ec7e42](https://github.com/gonicus/gonnect/commit/5ec7e4246ebc6a615f72a249b93df7faf926f292))

## [2.0.2](https://github.com/gonicus/gonnect/compare/v2.0.1...v2.0.2) (2025-08-15)


### Bug Fixes

* trigger new release ([aba7b2e](https://github.com/gonicus/gonnect/commit/aba7b2eeca7c76ba2044c00bc1df91f3a5901ba0))

## [2.0.1](https://github.com/gonicus/gonnect/compare/v2.0.0...v2.0.1) (2025-08-14)


### Bug Fixes

* togglers are working again (regression fix) ([#136](https://github.com/gonicus/gonnect/issues/136)) ([30fe9db](https://github.com/gonicus/gonnect/commit/30fe9db56cac40154ae1bf842a4ffcaeb382cf19))

# [2.0.0](https://github.com/gonicus/gonnect/compare/v1.3.12...v2.0.0) (2025-08-12)


### Features

* integration of Jitsi Meet and new single window design ([baf87f8](https://github.com/gonicus/gonnect/commit/baf87f821482b3d7383119410c8d370b16d0bbd3))


### BREAKING CHANGES

* Jitsi Meet session are now integrated. Upgrade seamlessly from a SIP call to a video conference.

## [1.3.12](https://github.com/gonicus/gonnect/compare/v1.3.11...v1.3.12) (2025-08-07)


### Bug Fixes

* only call handelIpChange when there is no active call ([1693600](https://github.com/gonicus/gonnect/commit/16936004748514a7b973e7f6b5f937381316cd24))

## [1.3.11](https://github.com/gonicus/gonnect/compare/v1.3.10...v1.3.11) (2025-07-16)


### Bug Fixes

* handle network changes by SIP stack ([#111](https://github.com/gonicus/gonnect/issues/111)) ([2437925](https://github.com/gonicus/gonnect/commit/2437925e964b33b63fd12873973204ab2c0f3a31))

## [1.3.10](https://github.com/gonicus/gonnect/compare/v1.3.9...v1.3.10) (2025-07-03)


### Bug Fixes

* add templates for easybell cloud pbx and trunk extensions ([#102](https://github.com/gonicus/gonnect/issues/102)) ([67f75ae](https://github.com/gonicus/gonnect/commit/67f75ae6241bd7729ab375a1e31b164b3b870f33))
* **deps:** updated hidapi ([#103](https://github.com/gonicus/gonnect/issues/103)) ([127995c](https://github.com/gonicus/gonnect/commit/127995c0661a1323dcc2dea521adade5b8bd6409))

## [1.3.9](https://github.com/gonicus/gonnect/compare/v1.3.8...v1.3.9) (2025-05-07)


### Bug Fixes

* do not send data to headset device if option is diabled ([e150c62](https://github.com/gonicus/gonnect/commit/e150c62b428fb65e6c900791a577ebe922ab6941))

## [1.3.8](https://github.com/gonicus/gonnect/compare/v1.3.7...v1.3.8) (2025-04-09)


### Bug Fixes

* allow configuration of RTP port range ([#74](https://github.com/gonicus/gonnect/issues/74)) ([762e45a](https://github.com/gonicus/gonnect/commit/762e45a6175c7b6eb32d4ad4ebdd3c8be062ed8e))
* show missed calls in tray icon ([#71](https://github.com/gonicus/gonnect/issues/71)) ([40e541a](https://github.com/gonicus/gonnect/commit/40e541abdfe614890e67cd676d34b4d6cb114879))

## [1.3.7](https://github.com/gonicus/gonnect/compare/v1.3.6...v1.3.7) (2025-03-31)


### Bug Fixes

* DBus initialization issues ([4991894](https://github.com/gonicus/gonnect/commit/4991894ca09750b0a04f37dac82a691c59e0fe4c))

## [1.3.6](https://github.com/gonicus/gonnect/compare/v1.3.5...v1.3.6) (2025-03-21)


### Bug Fixes

* trigger patch release ([42ac7dd](https://github.com/gonicus/gonnect/commit/42ac7ddb9fa53cd1cdb6ac1fedfa6af3b54244d1)), closes [#65](https://github.com/gonicus/gonnect/issues/65)

## [1.3.5](https://github.com/gonicus/gonnect/compare/v1.3.4...v1.3.5) (2025-03-07)


### Bug Fixes

* **hid:** take global type into account ([34ba99e](https://github.com/gonicus/gonnect/commit/34ba99e50982a94474d55fcc16ec625db9e6a755))
* read smaller HID events ([829713d](https://github.com/gonicus/gonnect/commit/829713ddefc1981ac0429fccea23f76fcd987675))
* support more headset devices ([879db54](https://github.com/gonicus/gonnect/commit/879db543b44e4cb26b9a7ea420b1753096329206))
* **ui:** use same source for avatar initials in call screen ([#53](https://github.com/gonicus/gonnect/issues/53)) ([099a4f4](https://github.com/gonicus/gonnect/commit/099a4f4c24e33fd23781e748b26b76e7274f236e))

## [1.3.4](https://github.com/gonicus/gonnect/compare/v1.3.3...v1.3.4) (2025-03-06)


### Bug Fixes

* only set hold LED, not hook on headsets ([#52](https://github.com/gonicus/gonnect/issues/52)) ([ca432d4](https://github.com/gonicus/gonnect/commit/ca432d4ea38dce9f13fb71c7553536775ecffee6))

## [1.3.3](https://github.com/gonicus/gonnect/compare/v1.3.2...v1.3.3) (2025-03-04)


### Bug Fixes

* add implementation for streaming light switch ([f5fa143](https://github.com/gonicus/gonnect/commit/f5fa143047910325590363349361f9226aafa4a1))
* add template for Telekom Magenta SIP ([#49](https://github.com/gonicus/gonnect/issues/49)) ([c84b42e](https://github.com/gonicus/gonnect/commit/c84b42ecf4dd65dc6e2fae0438de8ae546805d3c))
* added litra beam LX as busy light ([b0874d6](https://github.com/gonicus/gonnect/commit/b0874d65584cd5a6bc43dddbbcde88df961f332b))
* added litra beam LX as busy light ([9acc0f0](https://github.com/gonicus/gonnect/commit/9acc0f004019ec74abc9b0bc07027846283ace24))
* debounce hid writes on litra beam ([3c0e973](https://github.com/gonicus/gonnect/commit/3c0e973b9d8799a39d79ac5e2286b250c5d6b2a0))
* **ui:** re-evaluate number stats after contact added ([0e2d147](https://github.com/gonicus/gonnect/commit/0e2d1478228add1665c6c0660b30fd467766e917))

## [1.3.2](https://github.com/gonicus/gonnect/compare/v1.3.1...v1.3.2) (2025-02-28)


### Bug Fixes

* **ui:** re-evaluate number stats after contact added ([592636c](https://github.com/gonicus/gonnect/commit/592636c06bbb981fee0b62ccffaa285768afcdaf))
* use LDAP bind also for avatars ([5b58c0a](https://github.com/gonicus/gonnect/commit/5b58c0a00344225d6b73c3b833c0064422bba2cd)), closes [#43](https://github.com/gonicus/gonnect/issues/43)

## [1.3.1](https://github.com/gonicus/gonnect/compare/v1.3.0...v1.3.1) (2025-02-27)


### Bug Fixes

* **ui:** default to non-ssl LDAP connections ([5dbd0be](https://github.com/gonicus/gonnect/commit/5dbd0bebff0bb21f5cace50e24bf2525020b567b))

# [1.3.0](https://github.com/gonicus/gonnect/compare/v1.2.1...v1.3.0) (2025-02-27)


### Features

* custom CA file for LDAP TLS ([7a55040](https://github.com/gonicus/gonnect/commit/7a55040a4de7f02b34e3703f53390ce834a2881b)), closes [#21](https://github.com/gonicus/gonnect/issues/21)
* sasl/gssapi bind for ldap ([92eb024](https://github.com/gonicus/gonnect/commit/92eb02471051a54cb314b7c478bfadb195ab7e82)), closes [#21](https://github.com/gonicus/gonnect/issues/21)
* simple bind for ldap addressbook feeder ([07896ad](https://github.com/gonicus/gonnect/commit/07896ad0428b1de5acc5a8e5164fc2bcda8e200e)), closes [#21](https://github.com/gonicus/gonnect/issues/21)
* **ui:** show identity of forwarded participant ([f5d2474](https://github.com/gonicus/gonnect/commit/f5d2474f486eecb574dea564a16b6bf835f923a4))

## [1.2.1](https://github.com/gonicus/gonnect/compare/v1.2.0...v1.2.1) (2025-02-25)


### Bug Fixes

* add FRITZ!Box template ([82d92fb](https://github.com/gonicus/gonnect/commit/82d92fb85f1f583efdf1a396a091302003e6a3b2))
* add sipgate template ([f662df0](https://github.com/gonicus/gonnect/commit/f662df06418ecc9c0b3c2771b5a21a32025a3846))
* added presets ([d1a5bbc](https://github.com/gonicus/gonnect/commit/d1a5bbc06eadd86747b99b922113338f019aa9e3))
* allow setting of empty ua/nameservers ([45a4618](https://github.com/gonicus/gonnect/commit/45a46185575477bc7889cf054baf95b7cccdeb1f))
* avoid blocking mpris calls ([811d3f1](https://github.com/gonicus/gonnect/commit/811d3f1dafc03fdf0376e72bb88a61998401b2b2))
* re-enable "finish" button with preset values ([2f841c0](https://github.com/gonicus/gonnect/commit/2f841c0676de09518a9808af79acc480f22cf062))
* remove debug output ([925e0a2](https://github.com/gonicus/gonnect/commit/925e0a2f13460413c863c1ead1ece6ed786048a2))
* remove Telekom template, as is not read yet ([00fc066](https://github.com/gonicus/gonnect/commit/00fc066477d684dafe7286ef359e33e895c56407))
* use local nameservers by default ([22532dd](https://github.com/gonicus/gonnect/commit/22532dd29abf543deed6e416c404b262e376cc3e))

# [1.2.0](https://github.com/gonicus/gonnect/compare/v1.1.2...v1.2.0) (2025-02-20)


### Bug Fixes

* make mime-type based calls work again ([fed10c5](https://github.com/gonicus/gonnect/commit/fed10c51b2a68644cceb7f0886cdd66fa8bb86b2))
* make mime-type based calls work again ([56edb2c](https://github.com/gonicus/gonnect/commit/56edb2c9d21b24928e1f61ce2b6f5a56aee0cb31))


### Features

* support for usb busylights ([70da1ef](https://github.com/gonicus/gonnect/commit/70da1ef1a0130d8e478911171cbad19ed4d2aba1))
* support for usb busylights ([c5088c6](https://github.com/gonicus/gonnect/commit/c5088c6fabf00d3c0a03dc4c3da2bd9cf7580c4a))

## [1.1.2](https://github.com/gonicus/gonnect/compare/v1.1.1...v1.1.2) (2025-02-17)


### Bug Fixes

* address theoretical counter issue ([3f61bd5](https://github.com/gonicus/gonnect/commit/3f61bd508bf4bac0d698265f5668d6eb416f2285))
* really send call parameters to the main app ([abc1f43](https://github.com/gonicus/gonnect/commit/abc1f4341adbc79825b09df9541c95fb9ffc69f4))

## [1.1.1](https://github.com/gonicus/gonnect/compare/v1.1.0...v1.1.1) (2025-02-13)


### Bug Fixes

* re-read avatars after file names change ([41ed506](https://github.com/gonicus/gonnect/commit/41ed506e04b2fc153d11f8a6efc2d1a7f8cb9d17)), closes [#15](https://github.com/gonicus/gonnect/issues/15)

# [1.1.0](https://github.com/gonicus/gonnect/compare/v1.0.7...v1.1.0) (2025-02-12)


### Bug Fixes

* load avatars of contacts that are created later ([76f63ea](https://github.com/gonicus/gonnect/commit/76f63ea46db020277906935d5b5cc580000b3e04)), closes [#11](https://github.com/gonicus/gonnect/issues/11)
* remove direct jpeg deps ([41ff744](https://github.com/gonicus/gonnect/commit/41ff7440fee25a05c4b71e8c7dc8fe5b9ba802f3))
* send refresh signals after carddav contacts loaded ([f8a6f8f](https://github.com/gonicus/gonnect/commit/f8a6f8f9d23529864afd1e0fd374f57c45a09e3a)), closes [#11](https://github.com/gonicus/gonnect/issues/11)
* **ui:** send update signals when adding avatar via CardDAV ([87d5f8f](https://github.com/gonicus/gonnect/commit/87d5f8fdd7019d84fa2a154e4eab20d87acb82e4)), closes [#11](https://github.com/gonicus/gonnect/issues/11)


### Features

* CardDAV contact support ([b848a10](https://github.com/gonicus/gonnect/commit/b848a10e17b57de3b107caf2889fcea0c7976060)), closes [#11](https://github.com/gonicus/gonnect/issues/11)
* move to software clock by default and make it configurable ([690a04c](https://github.com/gonicus/gonnect/commit/690a04cee61cb3095596f8c62cff066f1779472b))

## [1.0.7](https://github.com/gonicus/gonnect/compare/v1.0.6...v1.0.7) (2025-01-20)


### Bug Fixes

* only use IM if allowed with SIP partner ([c183fe6](https://github.com/gonicus/gonnect/commit/c183fe60415623a3f5be1e11782af82b9f08fa00))

## [1.0.6](https://github.com/gonicus/gonnect/compare/v1.0.5...v1.0.6) (2025-01-16)


### Bug Fixes

* adjust screenshot shadow ([b934e09](https://github.com/gonicus/gonnect/commit/b934e096a1116bd956400045b8d71577a6ff7c74))
* remove "app" from summary ([81eb4b7](https://github.com/gonicus/gonnect/commit/81eb4b76e16a87f3331a0c512b24ab9ee830d7bb))

## [1.0.5](https://github.com/gonicus/gonnect/compare/v1.0.4...v1.0.5) (2025-01-14)


### Bug Fixes

* trigger build ([272b1e6](https://github.com/gonicus/gonnect/commit/272b1e674c951bf3ad40682820fbbc2aa0086551))

## [1.0.4](https://github.com/gonicus/gonnect/compare/v1.0.3...v1.0.4) (2025-01-13)


### Bug Fixes

* adjust file permissions of initial config ([e6beb1c](https://github.com/gonicus/gonnect/commit/e6beb1c060394550561a96aafc455756563c40f4))

## [1.0.3](https://github.com/gonicus/gonnect/compare/v1.0.2...v1.0.3) (2025-01-13)


### Bug Fixes

* adjust branding and icon size ([440d934](https://github.com/gonicus/gonnect/commit/440d934192f54c46c09c4e980d0086623d7ab04f))

## [1.0.2](https://github.com/gonicus/gonnect/compare/v1.0.1...v1.0.2) (2025-01-08)


### Bug Fixes

* add workaround for KDE tray ([46a5b50](https://github.com/gonicus/gonnect/commit/46a5b504c6801c0e35432798c10cd8473e175ea6))
* enable sample template ([00f5481](https://github.com/gonicus/gonnect/commit/00f54813147aab9e6911b3d01451fd028be5876d))
* properly show errors on template copy ([ff4bcb5](https://github.com/gonicus/gonnect/commit/ff4bcb5ada56c0091d366ef2873e196fa033dcc6))

## [1.0.1](https://github.com/gonicus/gonnect/compare/v1.0.0...v1.0.1) (2025-01-07)


### Bug Fixes

* don't try to show non existing avatars ([e740ae5](https://github.com/gonicus/gonnect/commit/e740ae5d105cea88e23be540c0108365c5289682))

# 1.0.0 (2024-12-19)


### Features

* initial commit ([50d290e](https://github.com/gonicus/gonnect/commit/50d290e5e57b62a3bec426a37811f889cb97cadf))
