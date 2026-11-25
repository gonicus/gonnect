# [2.5.0](https://github.com/gonicus/gonnect/compare/v2.4.1...v2.5.0) (2026-11-25)

### Bug Fixes

* add re-registration watchdog ([dea86c8](https://github.com/gonicus/gonnect/commit/dea86c8b834fff63530cce64661b4b7c2399b747))
* add transport to outgoing SIP URI ([7d03ed5](https://github.com/gonicus/gonnect/commit/7d03ed5655894dd0009aa250747c3143e55f8c89))
* add transport to SIP calls ([455f7f0](https://github.com/gonicus/gonnect/commit/455f7f0ccccc5fe09f65e3cd2a0b8b92c98f0dac))
* address crash on budy cleanup ([c6d9304](https://github.com/gonicus/gonnect/commit/c6d9304963493cfcdd211fed1477924ae1df8a9d))
* address headset audio crackling ([#478](https://github.com/gonicus/gonnect/issues/478)) ([c782e50](https://github.com/gonicus/gonnect/commit/c782e5083756236ba66e671e4eded60f1aa06c61))
* adjust widget scaling ([#635](https://github.com/gonicus/gonnect/issues/635)) ([668924d](https://github.com/gonicus/gonnect/commit/668924df6fe71928c8c3020c49239ff6f7b52d49))
* alignes presence state; EmojiPopup for KDE; multiple file/folder drag & drop ([#546](https://github.com/gonicus/gonnect/issues/546)) ([5f87238](https://github.com/gonicus/gonnect/commit/5f87238a3a11238ca8bdb68ee115f736375e1b03))
* allow multiple CFG and ENV placeholders in config ([#580](https://github.com/gonicus/gonnect/issues/580)) ([ed9f868](https://github.com/gonicus/gonnect/commit/ed9f868128ff47157fa5e8b3432a2f5a9b74626b))
* allow mwi without message count ([#538](https://github.com/gonicus/gonnect/issues/538)) ([887619a](https://github.com/gonicus/gonnect/commit/887619a4c7492f0f7a7961882af9e0bbe45aa6cb))
* async xfer ([#551](https://github.com/gonicus/gonnect/issues/551)) ([465338e](https://github.com/gonicus/gonnect/commit/465338e2682930f148442517eea12f584ec6fd00))
* avoid busy/hangup mixup in certain cases ([#583](https://github.com/gonicus/gonnect/issues/583)) ([9cb86b7](https://github.com/gonicus/gonnect/commit/9cb86b7ec3ba05a646c812677b09920093797792))
* base edits on SSL default configuration ([#459](https://github.com/gonicus/gonnect/issues/459)) ([d469791](https://github.com/gonicus/gonnect/commit/d46979135d6d70133f9af6e108a13742a9c545fd))
* better UI ([#574](https://github.com/gonicus/gonnect/issues/574)) ([8610abe](https://github.com/gonicus/gonnect/commit/8610abe15f95c873af070aa2b0422b543c14c56c))
* buddy agglomeration ([#812](https://github.com/gonicus/gonnect/issues/812)) ([da855b4](https://github.com/gonicus/gonnect/commit/da855b467ac44a55672acae64284a649cc8f9290))
* chat clipboard and chat room list section headers ([#568](https://github.com/gonicus/gonnect/issues/568)) ([42deb4e](https://github.com/gonicus/gonnect/commit/42deb4ea8e06c2806d620318a458f83bda62494b))
* chat fixes ([#543](https://github.com/gonicus/gonnect/issues/543)) ([4fd186b](https://github.com/gonicus/gonnect/commit/4fd186b48d72ad5904fe98d0bda0095a98efcaf3))
* chat small fixes and enhancements ([#509](https://github.com/gonicus/gonnect/issues/509)) ([55c437b](https://github.com/gonicus/gonnect/commit/55c437b6471bd61c72ca4b33aebfcfbd39d9b9cb))
* clear out URI encoded number input ([#475](https://github.com/gonicus/gonnect/issues/475)) ([3e48f2a](https://github.com/gonicus/gonnect/commit/3e48f2a85eb5179c777b14b6ad7cd5da2a95e54e))
* close knock participants window after all answered ([#692](https://github.com/gonicus/gonnect/issues/692)) ([19c7912](https://github.com/gonicus/gonnect/commit/19c7912e5007dfd8315c590db54b005d5a864d35))
* conan openssl mixup ([#612](https://github.com/gonicus/gonnect/issues/612)) ([7957b5f](https://github.com/gonicus/gonnect/commit/7957b5f537fcecaf9f20fa2ec806771cbe84dca7))
* continue webdav calendar queue on reply error ([#502](https://github.com/gonicus/gonnect/issues/502)) ([98a072c](https://github.com/gonicus/gonnect/commit/98a072c3b2ccacde7ea84b675c31f22c7da12b2f))
* crash on exit ([f29a6ca](https://github.com/gonicus/gonnect/commit/f29a6ca6f05f7036a22754b321f8a53e6b4cc66f))
* crashes when exiting during call/conference ([#767](https://github.com/gonicus/gonnect/issues/767)) ([1134a64](https://github.com/gonicus/gonnect/commit/1134a6407d875c8f98b6dab26c19e43009b08c2e))
* create direct chat room if not exists (via search) ([#624](https://github.com/gonicus/gonnect/issues/624)) ([4dc7ebf](https://github.com/gonicus/gonnect/commit/4dc7ebffc7061f037b198712d71c0df56086612c))
* dangling mwi pointer ([#682](https://github.com/gonicus/gonnect/issues/682)) ([ebfd06f](https://github.com/gonicus/gonnect/commit/ebfd06f8648ba821ebf33fe5a9de1095e059972f))
* date/address feed async issues ([b5d4f23](https://github.com/gonicus/gonnect/commit/b5d4f23cb5ca57d0785000bf49cc55a20ab570bb))
* disable PJSIP resolver cache ([4ec78c5](https://github.com/gonicus/gonnect/commit/4ec78c5d584228ee9d93c090e2600ee0271b61ec))
* disable setting external ringer ([#643](https://github.com/gonicus/gonnect/issues/643)) ([1eaecb0](https://github.com/gonicus/gonnect/commit/1eaecb0069c66736f431f14ecc4173959d572acd))
* do not send requests when IPC is disconnected ([#602](https://github.com/gonicus/gonnect/issues/602)) ([2dc88a5](https://github.com/gonicus/gonnect/commit/2dc88a54468ad2e1f791910f84b6285bf10e6838))
* do not show call page on incoming SIP during Jitsi conf ([#759](https://github.com/gonicus/gonnect/issues/759)) ([2c1e366](https://github.com/gonicus/gonnect/commit/2c1e3663939b7c3d5b098f9a4bd613983038377f))
* do not show favorites in "most called" list ([#456](https://github.com/gonicus/gonnect/issues/456)) ([787383b](https://github.com/gonicus/gonnect/commit/787383b1388e521f75e594256dac5483e90fddc3))
* do not wipe all contact sources on by-source removals ([#450](https://github.com/gonicus/gonnect/issues/450)) ([1a2763b](https://github.com/gonicus/gonnect/commit/1a2763bb691cc6bd53bcbed91d0dca35bc37eaf2))
* DTMF post dial ([#499](https://github.com/gonicus/gonnect/issues/499)) ([ad65e75](https://github.com/gonicus/gonnect/commit/ad65e754c320d7270900791f02e2516632d0dad6))
* emergency tooltip ([#804](https://github.com/gonicus/gonnect/issues/804)) ([0592ecc](https://github.com/gonicus/gonnect/commit/0592ecce5f64133e3041d8ba72e9f7c4608bd7d4))
* encrypted messages without content ([#504](https://github.com/gonicus/gonnect/issues/504)) ([ff8734d](https://github.com/gonicus/gonnect/commit/ff8734d2818a929defdb04e86c032a6afff91975))
* Ensure correct `tag` value type in flatpak manifest ([#466](https://github.com/gonicus/gonnect/issues/466)) ([e165209](https://github.com/gonicus/gonnect/commit/e165209fa0728abe481cd07bef7d91a065d5e583))
* extended fix/optimization collection ([#495](https://github.com/gonicus/gonnect/issues/495)) ([840eba1](https://github.com/gonicus/gonnect/commit/840eba1eaf853e922b97966c8fa383bcb01b9f76))
* favorite handling of conference rooms ([#737](https://github.com/gonicus/gonnect/issues/737)) ([1ad2718](https://github.com/gonicus/gonnect/commit/1ad27189a75ed88765b253b96f7585da989783e4))
* file path fixes and performance enhancements ([#550](https://github.com/gonicus/gonnect/issues/550)) ([e53fc17](https://github.com/gonicus/gonnect/commit/e53fc17cb901662aa2e82ad5bf536c8f6539ba82))
* fixed behavior when changing room while typing answer ([#677](https://github.com/gonicus/gonnect/issues/677)) ([840e649](https://github.com/gonicus/gonnect/commit/840e6495423450c9907b46a6121137a494b2c2fd))
* fixed chat message leak in IpcDispatcher ([#519](https://github.com/gonicus/gonnect/issues/519)) ([a8355b6](https://github.com/gonicus/gonnect/commit/a8355b6739c932ef95b1ec19492cdfb34fae37c1))
* fixed chat search result jumps with active thread ([#782](https://github.com/gonicus/gonnect/issues/782)) ([a8bab6e](https://github.com/gonicus/gonnect/commit/a8bab6edd38a789360604188d346a1bfb3393093))
* fixed crash because of empty stamp in jtisi message ([#582](https://github.com/gonicus/gonnect/issues/582)) ([b9b6aef](https://github.com/gonicus/gonnect/commit/b9b6aef78936c39856dd4801ed9b43de70c7ca57))
* fixed ctrl+f if no custom window decoration ([#717](https://github.com/gonicus/gonnect/issues/717)) ([c3f1630](https://github.com/gonicus/gonnect/commit/c3f1630db07eafdc73c7bdb5aaa5565f93f07a76))
* fixed popup positions on second screen ([#569](https://github.com/gonicus/gonnect/issues/569)) ([0aa3eb7](https://github.com/gonicus/gonnect/commit/0aa3eb7afe6a6bd8e7b8ddf65e174609ca817bd6))
* fixed sending typing ping after message has been sent ([#617](https://github.com/gonicus/gonnect/issues/617)) ([3434d5b](https://github.com/gonicus/gonnect/commit/3434d5b846fc871046cf84f834135ffac6eda8ad))
* fixed signals for chat room presence state updates ([#713](https://github.com/gonicus/gonnect/issues/713)) ([1824ad2](https://github.com/gonicus/gonnect/commit/1824ad28325869c2f932ca647f0ea4d679718435))
* fixed web profile usage of jitsi meet ([#810](https://github.com/gonicus/gonnect/issues/810)) ([7d97d37](https://github.com/gonicus/gonnect/commit/7d97d374614d19d4091f6ac9c25636f62ac246a9))
* fixed wrong Keys attached signal ([#766](https://github.com/gonicus/gonnect/issues/766)) ([307cda4](https://github.com/gonicus/gonnect/commit/307cda45671d8d958e02e4a9d63c61cb870c8430))
* fixes on calls and conferences ([#814](https://github.com/gonicus/gonnect/issues/814)) ([816f432](https://github.com/gonicus/gonnect/commit/816f43275c0c1104f5518919476a2aed2318461d))
* forward decryption of chat message so UI can show it ([#562](https://github.com/gonicus/gonnect/issues/562)) ([6ddbccb](https://github.com/gonicus/gonnect/commit/6ddbccb4b33e53bd10b8fabe276a073490045766))
* harmonize card/cal DAV handling ([aa99564](https://github.com/gonicus/gonnect/commit/aa995643520fd4cc7689b59bb52d2629a36ba807))
* hold on makel ([#535](https://github.com/gonicus/gonnect/issues/535)) ([a47de5d](https://github.com/gonicus/gonnect/commit/a47de5df13471bdc9569fb1d309c7b07fef13e12))
* **i18n:** fix Weblate quality issues ([#662](https://github.com/gonicus/gonnect/issues/662)) ([2be63ff](https://github.com/gonicus/gonnect/commit/2be63ff1703279529da9f3c32f06951140cec270))
* if so configured, do not verify CA even if they are state ([#455](https://github.com/gonicus/gonnect/issues/455)) ([7d1b34c](https://github.com/gonicus/gonnect/commit/7d1b34ccac09595c6c2276732e4606e78d2912de))
* ignore certain ssl errors when verifyServer is false (webdav) ([#451](https://github.com/gonicus/gonnect/issues/451)) ([6d23f49](https://github.com/gonicus/gonnect/commit/6d23f49846c3a4e25518c61a9ca026373eb46d35))
* ignore verification ssl errors if verifyServer=false ([#453](https://github.com/gonicus/gonnect/issues/453)) ([a04ed3a](https://github.com/gonicus/gonnect/commit/a04ed3ab162a057a9764b24a1fe8fc181bde7c3f))
* improve network handling ([59b791b](https://github.com/gonicus/gonnect/commit/59b791b3d76b57fcbd562a97d7c6ea275e5ea912))
* initial hook workaround ([#649](https://github.com/gonicus/gonnect/issues/649)) ([15af3ad](https://github.com/gonicus/gonnect/commit/15af3addd8f6212fcfa6ba4c1a5c3e41a36d2e0d))
* initial network state ([#606](https://github.com/gonicus/gonnect/issues/606)) ([634406c](https://github.com/gonicus/gonnect/commit/634406cb09cc7e04357db417b98fb4e4f9a26483))
* initialize streaming lights to "off" ([a6549cd](https://github.com/gonicus/gonnect/commit/a6549cd55ff002ef26c8532cbea01826f80acf8f))
* make addTransport const ([1f9bb48](https://github.com/gonicus/gonnect/commit/1f9bb489739d98802bbd8c820692f8b7facb0852))
* markdown fixes; CardDAV SIP subscribable attributes ([#584](https://github.com/gonicus/gonnect/issues/584)) ([0b4811c](https://github.com/gonicus/gonnect/commit/0b4811cb58e7376e86b15d502c9baff97d28f412))
* micro optimizations ([#791](https://github.com/gonicus/gonnect/issues/791)) ([3b5d7e5](https://github.com/gonicus/gonnect/commit/3b5d7e53d95c7ae216cd0a406994f2e908c5a3af))
* mini collection of issue fixes ([#775](https://github.com/gonicus/gonnect/issues/775)) ([bf39a61](https://github.com/gonicus/gonnect/commit/bf39a612778c9123b31a143ad10fa22bf8278967))
* mini fixes ([#560](https://github.com/gonicus/gonnect/issues/560)) ([6d06604](https://github.com/gonicus/gonnect/commit/6d066046ff0b957355f35719b4629a8b1484cadd))
* misc small stuff ([#785](https://github.com/gonicus/gonnect/issues/785)) ([d556fc7](https://github.com/gonicus/gonnect/commit/d556fc759a8f4b7b242858499f5b14cfcf4df8a1))
* more wide interpretation for the SIP URI ([42c2dcb](https://github.com/gonicus/gonnect/commit/42c2dcbd78ad3e86d461d6d033437bc1d4d6dce3))
* mute and flow optimizations ([#505](https://github.com/gonicus/gonnect/issues/505)) ([e8da2f8](https://github.com/gonicus/gonnect/commit/e8da2f8277550b853fedcfe207b90cccfc79e8ab))
* network issues ([#530](https://github.com/gonicus/gonnect/issues/530)) ([b3c87ed](https://github.com/gonicus/gonnect/commit/b3c87ed43612dd4fe5bde43c9cc2c4d46b6e6da9))
* new gouda connection states ([#541](https://github.com/gonicus/gonnect/issues/541)) ([991415e](https://github.com/gonicus/gonnect/commit/991415e0a892cd272fa6e0a7df78a36018fd1a03))
* no contact purge on CardDAV error ([#596](https://github.com/gonicus/gonnect/issues/596)) ([be8842e](https://github.com/gonicus/gonnect/commit/be8842eae71f45e0ce3ca8eb23305fc087c7cfa0))
* normalize phone numbers for history and favs the same way ([#636](https://github.com/gonicus/gonnect/issues/636)) ([43925d8](https://github.com/gonicus/gonnect/commit/43925d820239f2cb3ba81d9ae47c86c879b73aaa))
* number of small fixes ([#512](https://github.com/gonicus/gonnect/issues/512)) ([7634e9b](https://github.com/gonicus/gonnect/commit/7634e9be23610bb147fa670c71c8d6ca31eef23d))
* persist matrix pw ([#607](https://github.com/gonicus/gonnect/issues/607)) ([a5aca29](https://github.com/gonicus/gonnect/commit/a5aca29ac349ca2e5788eaae0ca2d03cd7327482))
* pjsip network change crash ([99ac619](https://github.com/gonicus/gonnect/commit/99ac619a25b08aa055926d8cdead9e5b1196bc2c))
* potential crash in tray menu ([987f2fc](https://github.com/gonicus/gonnect/commit/987f2fcb851abe3ae39bea66eca4a9520edca5d9))
* prevent unhold on currently changing call ([#439](https://github.com/gonicus/gonnect/issues/439)) ([ab47200](https://github.com/gonicus/gonnect/commit/ab472007d228913a29b9a314680cacdb46816ab8))
* queue credential jobs ([04902d4](https://github.com/gonicus/gonnect/commit/04902d405a87e5597af1527f60bf91d5e2b98274)), closes [#527](https://github.com/gonicus/gonnect/issues/527)
* queue credential jobs ([7c65584](https://github.com/gonicus/gonnect/commit/7c655842f7a5ad7b2b76d702f4eccc781bdda1f7)), closes [#527](https://github.com/gonicus/gonnect/issues/527)
* random crash on tear down ([#536](https://github.com/gonicus/gonnect/issues/536)) ([ea61a3f](https://github.com/gonicus/gonnect/commit/ea61a3f29ab03d468c0120e2ae130ab461d7f058))
* re-enable publish SIP setting ([#539](https://github.com/gonicus/gonnect/issues/539)) ([7e963a7](https://github.com/gonicus/gonnect/commit/7e963a7ca38929312e6f466924395d2343760a3f))
* read state of "raised hand" from jitsi api ([#716](https://github.com/gonicus/gonnect/issues/716)) ([1aa3e5c](https://github.com/gonicus/gonnect/commit/1aa3e5c999581d83031e312577b6e7c011f4d6ac))
* reduced read time out ([#725](https://github.com/gonicus/gonnect/issues/725)) ([5cf8805](https://github.com/gonicus/gonnect/commit/5cf8805b17bcea92946d07ea6a875d8719886049))
* refresh DNS resolver information ([ad25ba4](https://github.com/gonicus/gonnect/commit/ad25ba44cce4d4b70892358d19dab88f622042ae))
* remove deadlock ([#556](https://github.com/gonicus/gonnect/issues/556)) ([1215871](https://github.com/gonicus/gonnect/commit/1215871d1efb45a427f765c7a80a9df18795f7c0))
* remove trailing new line from message ([#605](https://github.com/gonicus/gonnect/issues/605)) ([f09fbc5](https://github.com/gonicus/gonnect/commit/f09fbc5551b34c28e20cf23c4fb1ed201efa0c9c))
* reorder messages on changing timestamp ([#691](https://github.com/gonicus/gonnect/issues/691)) ([bd11a6a](https://github.com/gonicus/gonnect/commit/bd11a6a9316c7b701ed4833cff7c9c8823129942))
* resolve ambigous contact lookup by source prio ([#674](https://github.com/gonicus/gonnect/issues/674)) ([dae0e8f](https://github.com/gonicus/gonnect/commit/dae0e8fa08c2057d58ec7128c42dcf8a328ec7fc))
* restore unsent message after editing another one ([#644](https://github.com/gonicus/gonnect/issues/644)) ([e638b42](https://github.com/gonicus/gonnect/commit/e638b42786242e3ceceb6c01efa3e2ff304d9c8f))
* robustness for media and contacts cache ([#471](https://github.com/gonicus/gonnect/issues/471)) ([c4265f7](https://github.com/gonicus/gonnect/commit/c4265f7b3952033fbfd60964bc07a48e98434ff6))
* room invitations ([#773](https://github.com/gonicus/gonnect/issues/773)) ([df0f0c5](https://github.com/gonicus/gonnect/commit/df0f0c500a9c96c2e5f97dac3cfc9f071d5799f2))
* room invitations whilst offline ([#610](https://github.com/gonicus/gonnect/issues/610)) ([9fdca81](https://github.com/gonicus/gonnect/commit/9fdca8192771fd2299005c0b96cfaaac6e4a7f60))
* rtt accitential re-enabled ([#571](https://github.com/gonicus/gonnect/issues/571)) ([72c0e38](https://github.com/gonicus/gonnect/commit/72c0e38ec189ac089877d9103fed3b78215dd810))
* same buddy state for all while resetting ([#768](https://github.com/gonicus/gonnect/issues/768)) ([1738060](https://github.com/gonicus/gonnect/commit/1738060e3d46101a86574ce3fcaa79be3226985f))
* saved tab bar order ([#554](https://github.com/gonicus/gonnect/issues/554)) ([20df8d4](https://github.com/gonicus/gonnect/commit/20df8d45c90959a23a1bf5aab378f00be4359739))
* small errors ([#816](https://github.com/gonicus/gonnect/issues/816)) ([1f3768e](https://github.com/gonicus/gonnect/commit/1f3768ea527ee1e8c3474ee379d0a0259730410e))
* small fixes when sending presence state via ipc ([#689](https://github.com/gonicus/gonnect/issues/689)) ([cbcb1e8](https://github.com/gonicus/gonnect/commit/cbcb1e857d03889b345a9121fc35270349c9e4e4))
* small UI fixes ([#589](https://github.com/gonicus/gonnect/issues/589)) ([5ea1898](https://github.com/gonicus/gonnect/commit/5ea18981d4d042033f47216eb154ae97bb6448f4))
* smaller chat fixes ([#529](https://github.com/gonicus/gonnect/issues/529)) ([f51ab00](https://github.com/gonicus/gonnect/commit/f51ab0073d4a29e682a9639c62ccacacdde3edef))
* streamlined context menu opening for 2nd screen ([#669](https://github.com/gonicus/gonnect/issues/669)) ([57eb49d](https://github.com/gonicus/gonnect/commit/57eb49d1485f0f3b7a53065e53954ac2cd31ca7a))
* subscribe SIPBuddy after the account is registered ([#779](https://github.com/gonicus/gonnect/issues/779)) ([ed4accd](https://github.com/gonicus/gonnect/commit/ed4accd44fbb9148ececd4bfac7b524f372de4ec))
* sync width calculation with upstream ([#684](https://github.com/gonicus/gonnect/issues/684)) ([cc604ff](https://github.com/gonicus/gonnect/commit/cc604ff696a9b068858e17711bbefcf88da7e8a6))
* threading mixups ([#711](https://github.com/gonicus/gonnect/issues/711)) ([9c9896a](https://github.com/gonicus/gonnect/commit/9c9896a0a181928fa5a45ca9ac6d648d208f998f))
* tiny UI issues ([#597](https://github.com/gonicus/gonnect/issues/597)) ([81ba9e0](https://github.com/gonicus/gonnect/commit/81ba9e089b089533a1c6a9a13fb99898d880c879))
* treat "no key found" not as error in Credentials ([#503](https://github.com/gonicus/gonnect/issues/503)) ([6df47d1](https://github.com/gonicus/gonnect/commit/6df47d1344075940ed4b219eba782afc0a3041cb))
* treat empty secret not as error ([#507](https://github.com/gonicus/gonnect/issues/507)) ([d3292e8](https://github.com/gonicus/gonnect/commit/d3292e868bddb31183bab03f51167773127677fe)), closes [#460](https://github.com/gonicus/gonnect/issues/460)
* trigger network change when from connected to connected ([#637](https://github.com/gonicus/gonnect/issues/637)) ([9abca39](https://github.com/gonicus/gonnect/commit/9abca395196ff281469dd69c1b64a506ffe18031))
* ui embellishments ([#702](https://github.com/gonicus/gonnect/issues/702)) ([2b1d0da](https://github.com/gonicus/gonnect/commit/2b1d0da75b82bfd30af5cf538d0b65054142dfab))
* ui polish ([#600](https://github.com/gonicus/gonnect/issues/600)) ([7095278](https://github.com/gonicus/gonnect/commit/7095278d98777a9f83900b6b639957209e0fda7f))
* **ui:** better behavior of chat room creation ([#593](https://github.com/gonicus/gonnect/issues/593)) ([150515e](https://github.com/gonicus/gonnect/commit/150515ec15dd58936706fd765ad003630598b358))
* **ui:** do not treat room names beginning with digits as phone numbers ([#461](https://github.com/gonicus/gonnect/issues/461)) ([648dfe5](https://github.com/gonicus/gonnect/commit/648dfe53e2ad43b1cb4af3a6b5967060dacfb030))
* **ui:** escape now closes popups ([#614](https://github.com/gonicus/gonnect/issues/614)) ([ab81b53](https://github.com/gonicus/gonnect/commit/ab81b53c7d7bc7c9e7d0dd088cf05c4b86d7083f))
* **ui:** fix narrow (portrait) videos in chat messages ([#709](https://github.com/gonicus/gonnect/issues/709)) ([baf47bf](https://github.com/gonicus/gonnect/commit/baf47bfd2ebfcf21819a2adcd6d376f4895c8369))
* **ui:** fixed blurry items due to sub-pixel placement ([#634](https://github.com/gonicus/gonnect/issues/634)) ([913c813](https://github.com/gonicus/gonnect/commit/913c813e93c5c7de44612a5f3ccf4b3efb2bdcff))
* **ui:** fixed disappearing section headers when leaving room ([#595](https://github.com/gonicus/gonnect/issues/595)) ([7f8f8ca](https://github.com/gonicus/gonnect/commit/7f8f8ca85650d0565186fc17586bb943469acdde))
* **ui:** fixed invite user filter after join/unjoin ([#566](https://github.com/gonicus/gonnect/issues/566)) ([754ae5c](https://github.com/gonicus/gonnect/commit/754ae5cb36e49331c59624f4f41bff3a24768ccd))
* **ui:** fixed larger numbers on BarButton's count label ([#749](https://github.com/gonicus/gonnect/issues/749)) ([3d783ae](https://github.com/gonicus/gonnect/commit/3d783ae6c0852330141062cf26637ba230bd8fcc))
* **ui:** fixed newlines when editing chat message ([#599](https://github.com/gonicus/gonnect/issues/599)) ([809d270](https://github.com/gonicus/gonnect/commit/809d270b1a952932c9a7901dd2ab549ce72cc4d6))
* **ui:** fixed overflowing buttons in LargeImage ([#603](https://github.com/gonicus/gonnect/issues/603)) ([27f8678](https://github.com/gonicus/gonnect/commit/27f8678cf8956e183d51461e3428df163990f420))
* **ui:** fixed person coin in three-party conference ([#564](https://github.com/gonicus/gonnect/issues/564)) ([938e3e5](https://github.com/gonicus/gonnect/commit/938e3e50778eae7f220503dc7759de579ff25d05))
* **ui:** fixed tooltip position so button remains clickable ([#728](https://github.com/gonicus/gonnect/issues/728)) ([06f5709](https://github.com/gonicus/gonnect/commit/06f5709c8305ab6e1f4ea6b2bf0ed90113f7f339))
* **ui:** icon for fav item with number type Unknown ([#557](https://github.com/gonicus/gonnect/issues/557)) ([ac86fd5](https://github.com/gonicus/gonnect/commit/ac86fd5ab1ff383a980d836ed5f8bc52f1d6667b))
* **ui:** make keyboard shortcuts work in web engine ([#615](https://github.com/gonicus/gonnect/issues/615)) ([93494eb](https://github.com/gonicus/gonnect/commit/93494eb8af934041cb0706fbb7392529795318f5))
* **ui:** prevent overlapping widgets ([#608](https://github.com/gonicus/gonnect/issues/608)) ([fad8b56](https://github.com/gonicus/gonnect/commit/fad8b56a121ba38625622beac6ae9900d6057c5d))
* **ui:** scroll to bottom whenver chat room changed ([#488](https://github.com/gonicus/gonnect/issues/488)) ([4e8d551](https://github.com/gonicus/gonnect/commit/4e8d551d547bf3e3e2597c71be3252add9b1f779))
* updated i18n ([#696](https://github.com/gonicus/gonnect/issues/696)) ([673ef20](https://github.com/gonicus/gonnect/commit/673ef206fea7eb43c4d3cab675a48b544612075f))
* use LDAP paged results ([#547](https://github.com/gonicus/gonnect/issues/547)) ([dcea4a0](https://github.com/gonicus/gonnect/commit/dcea4a087beb06a53be2943768cbeba9e6c3d262))
* use Shortcut to prevent focus issues ([#638](https://github.com/gonicus/gonnect/issues/638)) ([e569cfb](https://github.com/gonicus/gonnect/commit/e569cfb1c0569e1a4c8088766fd0295623eb0aa4))
* use verifyServer flag for CA verification with webdav ([#437](https://github.com/gonicus/gonnect/issues/437)) ([68e341f](https://github.com/gonicus/gonnect/commit/68e341fe5021394644b6e8a7f6021f1e44c84c5b))
* webwidget url fallback ([#726](https://github.com/gonicus/gonnect/issues/726)) ([23881c0](https://github.com/gonicus/gonnect/commit/23881c01b726632497013b45c2c3c47385a395b4))
* windows build protobuf/private ([925beaa](https://github.com/gonicus/gonnect/commit/925beaa82dbb59627d56e06db93efa94ea84ba1b))

### Features

* accessibility ([#743](https://github.com/gonicus/gonnect/issues/743)) ([387edf2](https://github.com/gonicus/gonnect/commit/387edf200bb0140fbd6d2dc24d50f8621f298e08))
* add Blinkstick busylight support ([#474](https://github.com/gonicus/gonnect/issues/474)) ([0bd24b8](https://github.com/gonicus/gonnect/commit/0bd24b8e0d48bb1f3e0ee0783a5fc6de94bd2b0b))
* add new config flag to adjust reg timeouts ([cfd1d5a](https://github.com/gonicus/gonnect/commit/cfd1d5a34e7955fdcb710d9785cc33111e5f63ff))
* add template for 1&1 ([#465](https://github.com/gonicus/gonnect/issues/465)) ([d1884b6](https://github.com/gonicus/gonnect/commit/d1884b6584b853fdace484596dffd0458fc6cf69))
* **carddav:** strip base numbers ([#561](https://github.com/gonicus/gonnect/issues/561)) ([9ffb83b](https://github.com/gonicus/gonnect/commit/9ffb83bc9119858a7e7f3414ca606d0673f466bb))
* chat as widget ([#659](https://github.com/gonicus/gonnect/issues/659)) ([e90e719](https://github.com/gonicus/gonnect/commit/e90e7192e7dc446f31f872028e92dcb65fdfa602))
* chat search ([#622](https://github.com/gonicus/gonnect/issues/622)) ([9a53861](https://github.com/gonicus/gonnect/commit/9a53861071a7a4798f6e5b3ad20278bc3e13a3bc))
* check file size and disallow to large files ([#604](https://github.com/gonicus/gonnect/issues/604)) ([597c475](https://github.com/gonicus/gonnect/commit/597c4758b0e866c72cf804a2ad29aa0c82f24826))
* conference link in chat room ([#738](https://github.com/gonicus/gonnect/issues/738)) ([f4f0465](https://github.com/gonicus/gonnect/commit/f4f046525970461c2f5e75545d56c9de3c7eaf05))
* debounce network changes ([7dda22f](https://github.com/gonicus/gonnect/commit/7dda22fd342129095ef75d767fea5d38da945527))
* debounce network changes ([c081455](https://github.com/gonicus/gonnect/commit/c081455d1fa92d7640005a3f734f94118def2102))
* filter for favorites widgets ([#781](https://github.com/gonicus/gonnect/issues/781)) ([918bae6](https://github.com/gonicus/gonnect/commit/918bae6dc2edb302f82df642c374a5faa13d6c96))
* flatpak portal plus ([#769](https://github.com/gonicus/gonnect/issues/769)) ([3e4b8e5](https://github.com/gonicus/gonnect/commit/3e4b8e5f12d0b087aa671bd4ae58d7bf7b11b87f))
* history rows can now be removed ([#657](https://github.com/gonicus/gonnect/issues/657)) ([f01c2d6](https://github.com/gonicus/gonnect/commit/f01c2d67a951f8fc6ec65a065e0d9319231d2d4e))
* ical rrule parsing optimization ([#457](https://github.com/gonicus/gonnect/issues/457)) ([eaad103](https://github.com/gonicus/gonnect/commit/eaad1037e0851cd4fbacc593cc57810a91424067))
* knock feature for jitsi meet ([#673](https://github.com/gonicus/gonnect/issues/673)) ([9476ba7](https://github.com/gonicus/gonnect/commit/9476ba77dd7fb203615545544ed1536670e3702b))
* **ldap:** configurable contact attribute mapping ([#445](https://github.com/gonicus/gonnect/issues/445)) ([a16c753](https://github.com/gonicus/gonnect/commit/a16c753fba4e5475641f6841940ed5128ada1c29))
* load older related messages ([#518](https://github.com/gonicus/gonnect/issues/518)) ([6b66aeb](https://github.com/gonicus/gonnect/commit/6b66aeb110ffc17298ab98118244cc1e81569cb1))
* marker for edited messages ([#723](https://github.com/gonicus/gonnect/issues/723)) ([0b4aa44](https://github.com/gonicus/gonnect/commit/0b4aa446c67f1dd1176e504c593b13a6aca25fbc))
* matrix chat in call screen ([#506](https://github.com/gonicus/gonnect/issues/506)) ([a9e2c45](https://github.com/gonicus/gonnect/commit/a9e2c4594a2564c08c2c5f1cd2e31773ddd7d19d))
* matrix notification settings ([#490](https://github.com/gonicus/gonnect/issues/490)) ([2fc462e](https://github.com/gonicus/gonnect/commit/2fc462e85710aa7c5495d87050848f0f86a734d4))
* matrix support via gouda-matrix ([#480](https://github.com/gonicus/gonnect/issues/480)) ([596aaff](https://github.com/gonicus/gonnect/commit/596aaff93932872a1677bcfcf36e39cfd64920ae))
* more real time prip and move ice to media thread ([#772](https://github.com/gonicus/gonnect/issues/772)) ([d619ed3](https://github.com/gonicus/gonnect/commit/d619ed3d3388e31d8645675c50e8edd4e8c32f33))
* more sip-chat-jitsi integration; small fixes ([#770](https://github.com/gonicus/gonnect/issues/770)) ([152cc0f](https://github.com/gonicus/gonnect/commit/152cc0f0d89c36f4e56ffafaa9b6c73fbf15674a))
* optimization collection ([#515](https://github.com/gonicus/gonnect/issues/515)) ([5929b18](https://github.com/gonicus/gonnect/commit/5929b182b8717612b227c0b107413e9d59a55878))
* own presence state in systray menu ([#497](https://github.com/gonicus/gonnect/issues/497)) ([6188a5b](https://github.com/gonicus/gonnect/commit/6188a5b1071ad37aee168a2566fd1e7a8e8eb44c))
* pinned messages ([#650](https://github.com/gonicus/gonnect/issues/650)) ([c7890a4](https://github.com/gonicus/gonnect/commit/c7890a4fc161e5abe9fc7ca9715fe31abac10437))
* read Diversion and History-Info ([#545](https://github.com/gonicus/gonnect/issues/545)) ([dfa6b6f](https://github.com/gonicus/gonnect/commit/dfa6b6f83fd643ab1cf5086886dfbcbe7a75fad0))
* read marker for chat messages ([#668](https://github.com/gonicus/gonnect/issues/668)) ([a00a1c4](https://github.com/gonicus/gonnect/commit/a00a1c4f451e3ee59e897e5ad3a289e7c2f1317d))
* rtt ui integration ([#443](https://github.com/gonicus/gonnect/issues/443)) ([af4121a](https://github.com/gonicus/gonnect/commit/af4121a89ee7119d170fd6637c67ac77e07532fe))
* save/restore filter value of activities widget ([#734](https://github.com/gonicus/gonnect/issues/734)) ([09f6ee5](https://github.com/gonicus/gonnect/commit/09f6ee532c5a3699deb7902922c2c3ef3e624704))
* show attached chat rooms in conference ([#751](https://github.com/gonicus/gonnect/issues/751)) ([1fef225](https://github.com/gonicus/gonnect/commit/1fef2252988320f21e097599ad4c7abaeaa2cd76))
* show direct chat after jitsi upgrade ([#627](https://github.com/gonicus/gonnect/issues/627)) ([afa39db](https://github.com/gonicus/gonnect/commit/afa39dba4733dd53bb2a2bd240c28f9edc70225a))
* show removed messages as such instead disappearing ([#653](https://github.com/gonicus/gonnect/issues/653)) ([560cc87](https://github.com/gonicus/gonnect/commit/560cc87567e23f1cd4f5a9512a239b596e0600d2))
* show unread marker in chat message list ([#722](https://github.com/gonicus/gonnect/issues/722)) ([cb84f49](https://github.com/gonicus/gonnect/commit/cb84f49dbc938f2d45537c625bca9ace19d9d479))
* show uploading state while larger message is pending ([#731](https://github.com/gonicus/gonnect/issues/731)) ([d3737f7](https://github.com/gonicus/gonnect/commit/d3737f77ddddcc1818897ca76a0177c5543b0277))
* small enhancements ([#524](https://github.com/gonicus/gonnect/issues/524)) ([887aff3](https://github.com/gonicus/gonnect/commit/887aff3bb64e9beb811e44c467bbe62796668aab))
* tag room with "[@room](https://github.com/room)" in message ([#618](https://github.com/gonicus/gonnect/issues/618)) ([8f41407](https://github.com/gonicus/gonnect/commit/8f414070204076c217315efef28657e9c3e19061))
* threads ([#660](https://github.com/gonicus/gonnect/issues/660)) ([07ad1f6](https://github.com/gonicus/gonnect/commit/07ad1f68481e1f3643a83a83fd5570d23f63b62f))
* transcription feature for Jitsi Meet conference ([#776](https://github.com/gonicus/gonnect/issues/776)) ([a52771c](https://github.com/gonicus/gonnect/commit/a52771c4229931a43566ee9ee9185f5f7c779554))
* uc chat call interlinks ([#655](https://github.com/gonicus/gonnect/issues/655)) ([827cc39](https://github.com/gonicus/gonnect/commit/827cc397a4bab9190ac5ee3b49cd1fc89509a318))
* **ui:** allow dtmf input in search box ([#609](https://github.com/gonicus/gonnect/issues/609)) ([51788a5](https://github.com/gonicus/gonnect/commit/51788a5e6dd44d0227b2266e6ef4d5724e74c602))
* **ui:** grouped favorites ([#491](https://github.com/gonicus/gonnect/issues/491)) ([3de4aac](https://github.com/gonicus/gonnect/commit/3de4aac7f9828a23cbeb990f47098b7e7c476cea)), closes [#494](https://github.com/gonicus/gonnect/issues/494)
* **ui:** leave conference via systray menu ([#438](https://github.com/gonicus/gonnect/issues/438)) ([f68554f](https://github.com/gonicus/gonnect/commit/f68554f1543d25ebe822cd7aa06834de194c41d5))
* **ui:** notification sound ([#510](https://github.com/gonicus/gonnect/issues/510)) ([f3e656f](https://github.com/gonicus/gonnect/commit/f3e656f23db376bc5a2d8964a524f8b6787711bf))
* **ui:** show user names of reactions in tooltip ([#656](https://github.com/gonicus/gonnect/issues/656)) ([b8d18f4](https://github.com/gonicus/gonnect/commit/b8d18f495ce4c6c32d10c15b2e9d45834aecfd7b))
* unified avatars from ldap and chat ([#628](https://github.com/gonicus/gonnect/issues/628)) ([c8da541](https://github.com/gonicus/gonnect/commit/c8da5418dfbd008016d5343924890f8e7dcbf142))
* verify ca flag for matrix chat ([#715](https://github.com/gonicus/gonnect/issues/715)) ([1199bbd](https://github.com/gonicus/gonnect/commit/1199bbda11be3d64dace65044ea0f73c22a5ac76))
* windows + msix ([#479](https://github.com/gonicus/gonnect/issues/479)) ([bf4f3b8](https://github.com/gonicus/gonnect/commit/bf4f3b87b40e956997b79c7561f3337bfddf1268))
* windows signing ([#462](https://github.com/gonicus/gonnect/issues/462)) ([58656cb](https://github.com/gonicus/gonnect/commit/58656cb6b5ec0eba71821f763301a8f2b50a91d7))

### Performance Improvements

* boost for initial matrix login ([#579](https://github.com/gonicus/gonnect/issues/579)) ([e9b1e3f](https://github.com/gonicus/gonnect/commit/e9b1e3fd857aa697db48b0b85779bdbadd024b0d))
* emojis ([#522](https://github.com/gonicus/gonnect/issues/522)) ([504eeb7](https://github.com/gonicus/gonnect/commit/504eeb7f53c9634968546cfb0598332902f88e1f))

### Reverts

* Revert "fix: improve network handling" ([67fd242](https://github.com/gonicus/gonnect/commit/67fd2421062b03d540ec0c5d141e496f0a61d4ae))

# [2.5.0-beta.11](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.10...v2.5.0-beta.11) (2026-11-16)

### Bug Fixes

* buddy agglomeration ([#812](https://github.com/gonicus/gonnect/issues/812)) ([da855b4](https://github.com/gonicus/gonnect/commit/da855b467ac44a55672acae64284a649cc8f9290))
* emergency tooltip ([#804](https://github.com/gonicus/gonnect/issues/804)) ([0592ecc](https://github.com/gonicus/gonnect/commit/0592ecce5f64133e3041d8ba72e9f7c4608bd7d4))
* fixed web profile usage of jitsi meet ([#810](https://github.com/gonicus/gonnect/issues/810)) ([7d97d37](https://github.com/gonicus/gonnect/commit/7d97d374614d19d4091f6ac9c25636f62ac246a9))
* fixes on calls and conferences ([#814](https://github.com/gonicus/gonnect/issues/814)) ([816f432](https://github.com/gonicus/gonnect/commit/816f43275c0c1104f5518919476a2aed2318461d))
* small errors ([#816](https://github.com/gonicus/gonnect/issues/816)) ([1f3768e](https://github.com/gonicus/gonnect/commit/1f3768ea527ee1e8c3474ee379d0a0259730410e))

# [2.5.0-beta.10](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.9...v2.5.0-beta.10) (2026-11-05)

### Bug Fixes

* fixed chat search result jumps with active thread ([#782](https://github.com/gonicus/gonnect/issues/782)) ([a8bab6e](https://github.com/gonicus/gonnect/commit/a8bab6edd38a789360604188d346a1bfb3393093))
* micro optimizations ([#791](https://github.com/gonicus/gonnect/issues/791)) ([3b5d7e5](https://github.com/gonicus/gonnect/commit/3b5d7e53d95c7ae216cd0a406994f2e908c5a3af))
* mini collection of issue fixes ([#775](https://github.com/gonicus/gonnect/issues/775)) ([bf39a61](https://github.com/gonicus/gonnect/commit/bf39a612778c9123b31a143ad10fa22bf8278967))
* misc small stuff ([#785](https://github.com/gonicus/gonnect/issues/785)) ([d556fc7](https://github.com/gonicus/gonnect/commit/d556fc759a8f4b7b242858499f5b14cfcf4df8a1))
* room invitations ([#773](https://github.com/gonicus/gonnect/issues/773)) ([df0f0c5](https://github.com/gonicus/gonnect/commit/df0f0c500a9c96c2e5f97dac3cfc9f071d5799f2))
* subscribe SIPBuddy after the account is registered ([#779](https://github.com/gonicus/gonnect/issues/779)) ([ed4accd](https://github.com/gonicus/gonnect/commit/ed4accd44fbb9148ececd4bfac7b524f372de4ec))


### Features

* chat search ([#622](https://github.com/gonicus/gonnect/issues/622)) ([9a53861](https://github.com/gonicus/gonnect/commit/9a53861071a7a4798f6e5b3ad20278bc3e13a3bc))
* filter for favorites widgets ([#781](https://github.com/gonicus/gonnect/issues/781)) ([918bae6](https://github.com/gonicus/gonnect/commit/918bae6dc2edb302f82df642c374a5faa13d6c96))
* transcription feature for Jitsi Meet conference ([#776](https://github.com/gonicus/gonnect/issues/776)) ([a52771c](https://github.com/gonicus/gonnect/commit/a52771c4229931a43566ee9ee9185f5f7c779554))

# [2.5.0-beta.9](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.8...v2.5.0-beta.9) (2026-10-15)


### Bug Fixes

* crashes when exiting during call/conference ([#767](https://github.com/gonicus/gonnect/issues/767)) ([1134a64](https://github.com/gonicus/gonnect/commit/1134a6407d875c8f98b6dab26c19e43009b08c2e))
* do not show call page on incoming SIP during Jitsi conf ([#759](https://github.com/gonicus/gonnect/issues/759)) ([2c1e366](https://github.com/gonicus/gonnect/commit/2c1e3663939b7c3d5b098f9a4bd613983038377f))
* same buddy state for all while resetting ([#768](https://github.com/gonicus/gonnect/issues/768)) ([1738060](https://github.com/gonicus/gonnect/commit/1738060e3d46101a86574ce3fcaa79be3226985f))


### Features

* flatpak portal plus ([#769](https://github.com/gonicus/gonnect/issues/769)) ([3e4b8e5](https://github.com/gonicus/gonnect/commit/3e4b8e5f12d0b087aa671bd4ae58d7bf7b11b87f))
* more real time prip and move ice to media thread ([#772](https://github.com/gonicus/gonnect/issues/772)) ([d619ed3](https://github.com/gonicus/gonnect/commit/d619ed3d3388e31d8645675c50e8edd4e8c32f33))
* more sip-chat-jitsi integration; small fixes ([#770](https://github.com/gonicus/gonnect/issues/770)) ([152cc0f](https://github.com/gonicus/gonnect/commit/152cc0f0d89c36f4e56ffafaa9b6c73fbf15674a))

# [2.5.0-beta.8](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.7...v2.5.0-beta.8) (2026-10-12)


### Bug Fixes

* favorite handling of conference rooms ([#737](https://github.com/gonicus/gonnect/issues/737)) ([1ad2718](https://github.com/gonicus/gonnect/commit/1ad27189a75ed88765b253b96f7585da989783e4))
* fixed wrong Keys attached signal ([#766](https://github.com/gonicus/gonnect/issues/766)) ([307cda4](https://github.com/gonicus/gonnect/commit/307cda45671d8d958e02e4a9d63c61cb870c8430))
* reduced read time out ([#725](https://github.com/gonicus/gonnect/issues/725)) ([5cf8805](https://github.com/gonicus/gonnect/commit/5cf8805b17bcea92946d07ea6a875d8719886049))
* **ui:** fixed larger numbers on BarButton's count label ([#749](https://github.com/gonicus/gonnect/issues/749)) ([3d783ae](https://github.com/gonicus/gonnect/commit/3d783ae6c0852330141062cf26637ba230bd8fcc))
* **ui:** fixed tooltip position so button remains clickable ([#728](https://github.com/gonicus/gonnect/issues/728)) ([06f5709](https://github.com/gonicus/gonnect/commit/06f5709c8305ab6e1f4ea6b2bf0ed90113f7f339))
* webwidget url fallback ([#726](https://github.com/gonicus/gonnect/issues/726)) ([23881c0](https://github.com/gonicus/gonnect/commit/23881c01b726632497013b45c2c3c47385a395b4))


### Features

* accessibility ([#743](https://github.com/gonicus/gonnect/issues/743)) ([387edf2](https://github.com/gonicus/gonnect/commit/387edf200bb0140fbd6d2dc24d50f8621f298e08))
* conference link in chat room ([#738](https://github.com/gonicus/gonnect/issues/738)) ([f4f0465](https://github.com/gonicus/gonnect/commit/f4f046525970461c2f5e75545d56c9de3c7eaf05))
* marker for edited messages ([#723](https://github.com/gonicus/gonnect/issues/723)) ([0b4aa44](https://github.com/gonicus/gonnect/commit/0b4aa446c67f1dd1176e504c593b13a6aca25fbc))
* save/restore filter value of activities widget ([#734](https://github.com/gonicus/gonnect/issues/734)) ([09f6ee5](https://github.com/gonicus/gonnect/commit/09f6ee532c5a3699deb7902922c2c3ef3e624704))
* show attached chat rooms in conference ([#751](https://github.com/gonicus/gonnect/issues/751)) ([1fef225](https://github.com/gonicus/gonnect/commit/1fef2252988320f21e097599ad4c7abaeaa2cd76))
* show unread marker in chat message list ([#722](https://github.com/gonicus/gonnect/issues/722)) ([cb84f49](https://github.com/gonicus/gonnect/commit/cb84f49dbc938f2d45537c625bca9ace19d9d479))
* show uploading state while larger message is pending ([#731](https://github.com/gonicus/gonnect/issues/731)) ([d3737f7](https://github.com/gonicus/gonnect/commit/d3737f77ddddcc1818897ca76a0177c5543b0277))
* threads ([#660](https://github.com/gonicus/gonnect/issues/660)) ([07ad1f6](https://github.com/gonicus/gonnect/commit/07ad1f68481e1f3643a83a83fd5570d23f63b62f))

# [2.5.0-beta.7](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.6...v2.5.0-beta.7) (2026-09-17)


### Bug Fixes

* fixed ctrl+f if no custom window decoration ([#717](https://github.com/gonicus/gonnect/issues/717)) ([c3f1630](https://github.com/gonicus/gonnect/commit/c3f1630db07eafdc73c7bdb5aaa5565f93f07a76))
* fixed signals for chat room presence state updates ([#713](https://github.com/gonicus/gonnect/issues/713)) ([1824ad2](https://github.com/gonicus/gonnect/commit/1824ad28325869c2f932ca647f0ea4d679718435))
* read state of "raised hand" from jitsi api ([#716](https://github.com/gonicus/gonnect/issues/716)) ([1aa3e5c](https://github.com/gonicus/gonnect/commit/1aa3e5c999581d83031e312577b6e7c011f4d6ac))
* threading mixups ([#711](https://github.com/gonicus/gonnect/issues/711)) ([9c9896a](https://github.com/gonicus/gonnect/commit/9c9896a0a181928fa5a45ca9ac6d648d208f998f))
* ui embellishments ([#702](https://github.com/gonicus/gonnect/issues/702)) ([2b1d0da](https://github.com/gonicus/gonnect/commit/2b1d0da75b82bfd30af5cf538d0b65054142dfab))
* **ui:** fix narrow (portrait) videos in chat messages ([#709](https://github.com/gonicus/gonnect/issues/709)) ([baf47bf](https://github.com/gonicus/gonnect/commit/baf47bfd2ebfcf21819a2adcd6d376f4895c8369))


### Features

* verify ca flag for matrix chat ([#715](https://github.com/gonicus/gonnect/issues/715)) ([1199bbd](https://github.com/gonicus/gonnect/commit/1199bbda11be3d64dace65044ea0f73c22a5ac76))


### Reverts

* Revert "fix: change active button color" ([b8f207d](https://github.com/gonicus/gonnect/commit/b8f207d9191e8fc8eeaaf108220ec393981a0728))

# [2.5.0-beta.6](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.5...v2.5.0-beta.6) (2026-09-10)


### Bug Fixes

* change active button color ([31ce2ea](https://github.com/gonicus/gonnect/commit/31ce2ea77597377ed93b1429ed20ea4c6895a747))

# [2.5.0-beta.5](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.4...v2.5.0-beta.5) (2026-09-09)


### Bug Fixes

* allow multiple CFG and ENV placeholders in config ([#580](https://github.com/gonicus/gonnect/issues/580)) ([58bb5c8](https://github.com/gonicus/gonnect/commit/58bb5c8de93730b1f019f9094eaee87216a7fb4d))
* close knock participants window after all answered ([#692](https://github.com/gonicus/gonnect/issues/692)) ([19c7912](https://github.com/gonicus/gonnect/commit/19c7912e5007dfd8315c590db54b005d5a864d35))
* dangling mwi pointer ([#682](https://github.com/gonicus/gonnect/issues/682)) ([ebfd06f](https://github.com/gonicus/gonnect/commit/ebfd06f8648ba821ebf33fe5a9de1095e059972f))
* fixed behavior when changing room while typing answer ([#677](https://github.com/gonicus/gonnect/issues/677)) ([840e649](https://github.com/gonicus/gonnect/commit/840e6495423450c9907b46a6121137a494b2c2fd))
* fixed crash because of empty stamp in jtisi message ([#582](https://github.com/gonicus/gonnect/issues/582)) ([075cd00](https://github.com/gonicus/gonnect/commit/075cd00469cdd8a451ae0e64f2febca479d53e2d))
* initial network state ([#606](https://github.com/gonicus/gonnect/issues/606)) ([4515265](https://github.com/gonicus/gonnect/commit/45152659c82709d145f00ec312291e32cfdddb5b))
* no contact purge on CardDAV error ([#596](https://github.com/gonicus/gonnect/issues/596)) ([ef06791](https://github.com/gonicus/gonnect/commit/ef0679135a14b1a1920fb8e566646f01ee58b900))
* reorder messages on changing timestamp ([#691](https://github.com/gonicus/gonnect/issues/691)) ([bd11a6a](https://github.com/gonicus/gonnect/commit/bd11a6a9316c7b701ed4833cff7c9c8823129942))
* resolve ambigous contact lookup by source prio ([#674](https://github.com/gonicus/gonnect/issues/674)) ([dae0e8f](https://github.com/gonicus/gonnect/commit/dae0e8fa08c2057d58ec7128c42dcf8a328ec7fc))
* small fixes when sending presence state via ipc ([#689](https://github.com/gonicus/gonnect/issues/689)) ([cbcb1e8](https://github.com/gonicus/gonnect/commit/cbcb1e857d03889b345a9121fc35270349c9e4e4))
* streamlined context menu opening for 2nd screen ([#669](https://github.com/gonicus/gonnect/issues/669)) ([57eb49d](https://github.com/gonicus/gonnect/commit/57eb49d1485f0f3b7a53065e53954ac2cd31ca7a))
* sync width calculation with upstream ([#684](https://github.com/gonicus/gonnect/issues/684)) ([cc604ff](https://github.com/gonicus/gonnect/commit/cc604ff696a9b068858e17711bbefcf88da7e8a6))
* updated i18n ([#696](https://github.com/gonicus/gonnect/issues/696)) ([673ef20](https://github.com/gonicus/gonnect/commit/673ef206fea7eb43c4d3cab675a48b544612075f))


### Features

* chat as widget ([#659](https://github.com/gonicus/gonnect/issues/659)) ([e90e719](https://github.com/gonicus/gonnect/commit/e90e7192e7dc446f31f872028e92dcb65fdfa602))
* knock feature for jitsi meet ([#673](https://github.com/gonicus/gonnect/issues/673)) ([9476ba7](https://github.com/gonicus/gonnect/commit/9476ba77dd7fb203615545544ed1536670e3702b))
* read marker for chat messages ([#668](https://github.com/gonicus/gonnect/issues/668)) ([a00a1c4](https://github.com/gonicus/gonnect/commit/a00a1c4f451e3ee59e897e5ad3a289e7c2f1317d))

# [2.5.0-beta.4](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.3...v2.5.0-beta.4) (2026-08-25)


### Bug Fixes

* disable setting external ringer ([#643](https://github.com/gonicus/gonnect/issues/643)) ([1eaecb0](https://github.com/gonicus/gonnect/commit/1eaecb0069c66736f431f14ecc4173959d572acd))
* **i18n:** fix Weblate quality issues ([#662](https://github.com/gonicus/gonnect/issues/662)) ([2be63ff](https://github.com/gonicus/gonnect/commit/2be63ff1703279529da9f3c32f06951140cec270))
* initial hook workaround ([#649](https://github.com/gonicus/gonnect/issues/649)) ([15af3ad](https://github.com/gonicus/gonnect/commit/15af3addd8f6212fcfa6ba4c1a5c3e41a36d2e0d))
* normalize phone numbers for history and favs the same way ([#636](https://github.com/gonicus/gonnect/issues/636)) ([43925d8](https://github.com/gonicus/gonnect/commit/43925d820239f2cb3ba81d9ae47c86c879b73aaa))
* restore unsent message after editing another one ([#644](https://github.com/gonicus/gonnect/issues/644)) ([e638b42](https://github.com/gonicus/gonnect/commit/e638b42786242e3ceceb6c01efa3e2ff304d9c8f))


### Features

* history rows can now be removed ([#657](https://github.com/gonicus/gonnect/issues/657)) ([f01c2d6](https://github.com/gonicus/gonnect/commit/f01c2d67a951f8fc6ec65a065e0d9319231d2d4e))
* pinned messages ([#650](https://github.com/gonicus/gonnect/issues/650)) ([c7890a4](https://github.com/gonicus/gonnect/commit/c7890a4fc161e5abe9fc7ca9715fe31abac10437))
* show removed messages as such instead disappearing ([#653](https://github.com/gonicus/gonnect/issues/653)) ([560cc87](https://github.com/gonicus/gonnect/commit/560cc87567e23f1cd4f5a9512a239b596e0600d2))
* uc chat call interlinks ([#655](https://github.com/gonicus/gonnect/issues/655)) ([827cc39](https://github.com/gonicus/gonnect/commit/827cc397a4bab9190ac5ee3b49cd1fc89509a318))
* **ui:** show user names of reactions in tooltip ([#656](https://github.com/gonicus/gonnect/issues/656)) ([b8d18f4](https://github.com/gonicus/gonnect/commit/b8d18f495ce4c6c32d10c15b2e9d45834aecfd7b))

# [2.5.0-beta.3](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.2...v2.5.0-beta.3) (2026-08-07)


### Bug Fixes

* adjust widget scaling ([#635](https://github.com/gonicus/gonnect/issues/635)) ([668924d](https://github.com/gonicus/gonnect/commit/668924df6fe71928c8c3020c49239ff6f7b52d49))
* trigger network change when from connected to connected ([#637](https://github.com/gonicus/gonnect/issues/637)) ([9abca39](https://github.com/gonicus/gonnect/commit/9abca395196ff281469dd69c1b64a506ffe18031))
* **ui:** fixed blurry items due to sub-pixel placement ([#634](https://github.com/gonicus/gonnect/issues/634)) ([913c813](https://github.com/gonicus/gonnect/commit/913c813e93c5c7de44612a5f3ccf4b3efb2bdcff))
* use Shortcut to prevent focus issues ([#638](https://github.com/gonicus/gonnect/issues/638)) ([e569cfb](https://github.com/gonicus/gonnect/commit/e569cfb1c0569e1a4c8088766fd0295623eb0aa4))


### Features

* show direct chat after jitsi upgrade ([#627](https://github.com/gonicus/gonnect/issues/627)) ([afa39db](https://github.com/gonicus/gonnect/commit/afa39dba4733dd53bb2a2bd240c28f9edc70225a))
* unified avatars from ldap and chat ([#628](https://github.com/gonicus/gonnect/issues/628)) ([c8da541](https://github.com/gonicus/gonnect/commit/c8da5418dfbd008016d5343924890f8e7dcbf142))

# [2.5.0-beta.2](https://github.com/gonicus/gonnect/compare/v2.5.0-beta.1...v2.5.0-beta.2) (2026-08-04)


### Bug Fixes

* allow multiple CFG and ENV placeholders in config ([#580](https://github.com/gonicus/gonnect/issues/580)) ([ed9f868](https://github.com/gonicus/gonnect/commit/ed9f868128ff47157fa5e8b3432a2f5a9b74626b))
* avoid busy/hangup mixup in certain cases ([#583](https://github.com/gonicus/gonnect/issues/583)) ([9cb86b7](https://github.com/gonicus/gonnect/commit/9cb86b7ec3ba05a646c812677b09920093797792))
* conan openssl mixup ([#612](https://github.com/gonicus/gonnect/issues/612)) ([7957b5f](https://github.com/gonicus/gonnect/commit/7957b5f537fcecaf9f20fa2ec806771cbe84dca7))
* create direct chat room if not exists (via search) ([#624](https://github.com/gonicus/gonnect/issues/624)) ([4dc7ebf](https://github.com/gonicus/gonnect/commit/4dc7ebffc7061f037b198712d71c0df56086612c))
* do not send requests when IPC is disconnected ([#602](https://github.com/gonicus/gonnect/issues/602)) ([2dc88a5](https://github.com/gonicus/gonnect/commit/2dc88a54468ad2e1f791910f84b6285bf10e6838))
* fixed crash because of empty stamp in jtisi message ([#582](https://github.com/gonicus/gonnect/issues/582)) ([b9b6aef](https://github.com/gonicus/gonnect/commit/b9b6aef78936c39856dd4801ed9b43de70c7ca57))
* fixed sending typing ping after message has been sent ([#617](https://github.com/gonicus/gonnect/issues/617)) ([3434d5b](https://github.com/gonicus/gonnect/commit/3434d5b846fc871046cf84f834135ffac6eda8ad))
* initial network state ([#606](https://github.com/gonicus/gonnect/issues/606)) ([634406c](https://github.com/gonicus/gonnect/commit/634406cb09cc7e04357db417b98fb4e4f9a26483))
* markdown fixes; CardDAV SIP subscribable attributes ([#584](https://github.com/gonicus/gonnect/issues/584)) ([0b4811c](https://github.com/gonicus/gonnect/commit/0b4811cb58e7376e86b15d502c9baff97d28f412))
* no contact purge on CardDAV error ([#596](https://github.com/gonicus/gonnect/issues/596)) ([be8842e](https://github.com/gonicus/gonnect/commit/be8842eae71f45e0ce3ca8eb23305fc087c7cfa0))
* persist matrix pw ([#607](https://github.com/gonicus/gonnect/issues/607)) ([a5aca29](https://github.com/gonicus/gonnect/commit/a5aca29ac349ca2e5788eaae0ca2d03cd7327482))
* remove trailing new line from message ([#605](https://github.com/gonicus/gonnect/issues/605)) ([f09fbc5](https://github.com/gonicus/gonnect/commit/f09fbc5551b34c28e20cf23c4fb1ed201efa0c9c))
* room invitations whilst offline ([#610](https://github.com/gonicus/gonnect/issues/610)) ([9fdca81](https://github.com/gonicus/gonnect/commit/9fdca8192771fd2299005c0b96cfaaac6e4a7f60))
* small UI fixes ([#589](https://github.com/gonicus/gonnect/issues/589)) ([5ea1898](https://github.com/gonicus/gonnect/commit/5ea18981d4d042033f47216eb154ae97bb6448f4))
* tiny UI issues ([#597](https://github.com/gonicus/gonnect/issues/597)) ([81ba9e0](https://github.com/gonicus/gonnect/commit/81ba9e089b089533a1c6a9a13fb99898d880c879))
* ui polish ([#600](https://github.com/gonicus/gonnect/issues/600)) ([7095278](https://github.com/gonicus/gonnect/commit/7095278d98777a9f83900b6b639957209e0fda7f))
* **ui:** better behavior of chat room creation ([#593](https://github.com/gonicus/gonnect/issues/593)) ([150515e](https://github.com/gonicus/gonnect/commit/150515ec15dd58936706fd765ad003630598b358))
* **ui:** escape now closes popups ([#614](https://github.com/gonicus/gonnect/issues/614)) ([ab81b53](https://github.com/gonicus/gonnect/commit/ab81b53c7d7bc7c9e7d0dd088cf05c4b86d7083f))
* **ui:** fixed disappearing section headers when leaving room ([#595](https://github.com/gonicus/gonnect/issues/595)) ([7f8f8ca](https://github.com/gonicus/gonnect/commit/7f8f8ca85650d0565186fc17586bb943469acdde))
* **ui:** fixed newlines when editing chat message ([#599](https://github.com/gonicus/gonnect/issues/599)) ([809d270](https://github.com/gonicus/gonnect/commit/809d270b1a952932c9a7901dd2ab549ce72cc4d6))
* **ui:** fixed overflowing buttons in LargeImage ([#603](https://github.com/gonicus/gonnect/issues/603)) ([27f8678](https://github.com/gonicus/gonnect/commit/27f8678cf8956e183d51461e3428df163990f420))
* **ui:** make keyboard shortcuts work in web engine ([#615](https://github.com/gonicus/gonnect/issues/615)) ([93494eb](https://github.com/gonicus/gonnect/commit/93494eb8af934041cb0706fbb7392529795318f5))
* **ui:** prevent overlapping widgets ([#608](https://github.com/gonicus/gonnect/issues/608)) ([fad8b56](https://github.com/gonicus/gonnect/commit/fad8b56a121ba38625622beac6ae9900d6057c5d))


### Features

* check file size and disallow to large files ([#604](https://github.com/gonicus/gonnect/issues/604)) ([597c475](https://github.com/gonicus/gonnect/commit/597c4758b0e866c72cf804a2ad29aa0c82f24826))
* tag room with "[@room](https://github.com/room)" in message ([#618](https://github.com/gonicus/gonnect/issues/618)) ([8f41407](https://github.com/gonicus/gonnect/commit/8f414070204076c217315efef28657e9c3e19061))
* **ui:** allow dtmf input in search box ([#609](https://github.com/gonicus/gonnect/issues/609)) ([51788a5](https://github.com/gonicus/gonnect/commit/51788a5e6dd44d0227b2266e6ef4d5724e74c602))


### Performance Improvements

* boost for initial matrix login ([#579](https://github.com/gonicus/gonnect/issues/579)) ([e9b1e3f](https://github.com/gonicus/gonnect/commit/e9b1e3fd857aa697db48b0b85779bdbadd024b0d))

## [2.4.1](https://github.com/gonicus/gonnect/compare/v2.4.0...v2.4.1) (2026-07-31)


### Bug Fixes

* allow multiple CFG and ENV placeholders in config ([#580](https://github.com/gonicus/gonnect/issues/580)) ([58bb5c8](https://github.com/gonicus/gonnect/commit/58bb5c8de93730b1f019f9094eaee87216a7fb4d))
* fixed crash because of empty stamp in jtisi message ([#582](https://github.com/gonicus/gonnect/issues/582)) ([075cd00](https://github.com/gonicus/gonnect/commit/075cd00469cdd8a451ae0e64f2febca479d53e2d))
* initial network state ([#606](https://github.com/gonicus/gonnect/issues/606)) ([4515265](https://github.com/gonicus/gonnect/commit/45152659c82709d145f00ec312291e32cfdddb5b))
* no contact purge on CardDAV error ([#596](https://github.com/gonicus/gonnect/issues/596)) ([ef06791](https://github.com/gonicus/gonnect/commit/ef0679135a14b1a1920fb8e566646f01ee58b900))

# [2.4.0](https://github.com/gonicus/gonnect/compare/v2.3.2...v2.4.0) (2026-07-07)


### Bug Fixes

* async xfer ([#551](https://github.com/gonicus/gonnect/issues/551)) ([f25cc6a](https://github.com/gonicus/gonnect/commit/f25cc6a6957c0a9c808fd013e93e2efcf511fe00))
* use LDAP paged results ([#547](https://github.com/gonicus/gonnect/issues/547)) ([f37b94d](https://github.com/gonicus/gonnect/commit/f37b94db2d6ef9f7d90b329eacd03519a93cbfb6))


### Features

* **carddav:** strip base numbers ([#561](https://github.com/gonicus/gonnect/issues/561)) ([bb3c806](https://github.com/gonicus/gonnect/commit/bb3c806e3a8e38d308dd87acaf30f5f269ab449a))

## [2.3.2](https://github.com/gonicus/gonnect/compare/v2.3.1...v2.3.2) (2026-07-01)


### Bug Fixes

* address crash on budy cleanup ([30ef127](https://github.com/gonicus/gonnect/commit/30ef127213511814de481b1083f3e637c67b87ea))
* allow mwi without message count ([#538](https://github.com/gonicus/gonnect/issues/538)) ([574d5e1](https://github.com/gonicus/gonnect/commit/574d5e1fcbb958656bea57a67504a05a516d2e0c))
* continue webdav calendar queue on reply error ([#502](https://github.com/gonicus/gonnect/issues/502)) ([f680ec0](https://github.com/gonicus/gonnect/commit/f680ec0dd8635a376b5f231449c21b416103b7ce))
* hold on makel ([#535](https://github.com/gonicus/gonnect/issues/535)) ([215376e](https://github.com/gonicus/gonnect/commit/215376ed1a59821c31f9ba7c52981955bb2bda9a))

## [2.3.1](https://github.com/gonicus/gonnect/compare/v2.3.0...v2.3.1) (2026-06-29)


### Bug Fixes

* network issues ([#530](https://github.com/gonicus/gonnect/issues/530)) ([e67d250](https://github.com/gonicus/gonnect/commit/e67d250ed08c840ef909b67863bd54dc0713b871))
* queue credential jobs ([c1461bf](https://github.com/gonicus/gonnect/commit/c1461bfca6915724a62f73530c607827fbef8194)), closes [#527](https://github.com/gonicus/gonnect/issues/527)

# [2.3.0](https://github.com/gonicus/gonnect/compare/v2.2.0...v2.3.0) (2026-06-25)


### Bug Fixes

* add transport to SIP calls ([8d6cb7a](https://github.com/gonicus/gonnect/commit/8d6cb7a869ccb47733e8ef4be2c76bed8c0c17b4))
* hidapi build with new libusb ([ef15f64](https://github.com/gonicus/gonnect/commit/ef15f64359286661fd04bc36c8435db74410a0b2))
* improve network handling ([9ea7748](https://github.com/gonicus/gonnect/commit/9ea7748addb9e7da240205bd4bf94d89fe1bf2e0))
* make addTransport const ([6e1a81d](https://github.com/gonicus/gonnect/commit/6e1a81d7ec6049c44b0cf4ac60490a704d8fb840))
* make preferredCodecs work ([5836d58](https://github.com/gonicus/gonnect/commit/5836d58e1ccfa75e99ec8048943c407e5fafa8f3))
* pjsip network change crash ([f6b615f](https://github.com/gonicus/gonnect/commit/f6b615f203df797c1276f5b17974e9c26f0e6e65))


### Features

* debounce network changes ([0a02ac1](https://github.com/gonicus/gonnect/commit/0a02ac11117275b20ebccd2906209412852dc6b7))

## [2.2.1](https://github.com/gonicus/gonnect/compare/v2.2.0...v2.2.1) (2026-06-25)


### Bug Fixes

* add transport to SIP calls ([8d6cb7a](https://github.com/gonicus/gonnect/commit/8d6cb7a869ccb47733e8ef4be2c76bed8c0c17b4))
* hidapi build with new libusb ([ef15f64](https://github.com/gonicus/gonnect/commit/ef15f64359286661fd04bc36c8435db74410a0b2))
* improve network handling ([9ea7748](https://github.com/gonicus/gonnect/commit/9ea7748addb9e7da240205bd4bf94d89fe1bf2e0))
* make addTransport const ([6e1a81d](https://github.com/gonicus/gonnect/commit/6e1a81d7ec6049c44b0cf4ac60490a704d8fb840))
* make preferredCodecs work ([5836d58](https://github.com/gonicus/gonnect/commit/5836d58e1ccfa75e99ec8048943c407e5fafa8f3))
* pjsip network change crash ([f6b615f](https://github.com/gonicus/gonnect/commit/f6b615f203df797c1276f5b17974e9c26f0e6e65))

# [2.2.0](https://github.com/gonicus/gonnect/compare/v2.1.0...v2.2.0) (2026-06-01)


### Bug Fixes

* base edits on SSL default configuration ([#459](https://github.com/gonicus/gonnect/issues/459)) ([6dcbf78](https://github.com/gonicus/gonnect/commit/6dcbf78a5b1b470ec966d8704b68e7d3e21da490))
* build with certificate patch ([#433](https://github.com/gonicus/gonnect/issues/433)) ([8e8ba1f](https://github.com/gonicus/gonnect/commit/8e8ba1f9897457a4582ffdb059985569ac957984))
* c++ standard and guarding updates ([#422](https://github.com/gonicus/gonnect/issues/422)) ([9e8171b](https://github.com/gonicus/gonnect/commit/9e8171ba4ef5336e988d6938fa62793432d89ae5))
* clear out URI encoded number input ([#475](https://github.com/gonicus/gonnect/issues/475)) ([cd1c126](https://github.com/gonicus/gonnect/commit/cd1c126b55aef57532db88ae432705d8421f7fe7))
* code review loop ([#374](https://github.com/gonicus/gonnect/issues/374)) ([f1c609d](https://github.com/gonicus/gonnect/commit/f1c609da8dc0214e2f0c4909ef7b645b08352c11))
* disable WMME in pjsip ([#397](https://github.com/gonicus/gonnect/issues/397)) ([a90a80c](https://github.com/gonicus/gonnect/commit/a90a80cb4796c96f1626f82d3e59d49312c23585))
* do not show favorites in "most called" list ([#456](https://github.com/gonicus/gonnect/issues/456)) ([8ee676d](https://github.com/gonicus/gonnect/commit/8ee676d08a6e79eba5c31749fb62531dfa6c7069))
* do not wipe all contact sources on by-source removals ([#450](https://github.com/gonicus/gonnect/issues/450)) ([f89882f](https://github.com/gonicus/gonnect/commit/f89882f9be298c1f90de881239b44a50b749492b))
* Ensure correct `tag` value type in flatpak manifest ([#466](https://github.com/gonicus/gonnect/issues/466)) ([3c5412a](https://github.com/gonicus/gonnect/commit/3c5412a250d5a7e3c3404432ae57d28465bac48c))
* handle direct pjsip mute again ([#402](https://github.com/gonicus/gonnect/issues/402)) ([0c4609c](https://github.com/gonicus/gonnect/commit/0c4609ca355f760478a19bda2c618aa299ff86eb))
* home/call switch tab notification reset ([#404](https://github.com/gonicus/gonnect/issues/404)) ([95e0a3f](https://github.com/gonicus/gonnect/commit/95e0a3ff849fda5a6fbb7e2294a99349edef77f8))
* if so configured, do not verify CA even if they are state ([#455](https://github.com/gonicus/gonnect/issues/455)) ([f45a1d8](https://github.com/gonicus/gonnect/commit/f45a1d81f152bb8d2024dad675206302255e2126))
* ignore certain ssl errors when verifyServer is false (webdav) ([#451](https://github.com/gonicus/gonnect/issues/451)) ([428860a](https://github.com/gonicus/gonnect/commit/428860a320785b9a52888a824d28990f59a6beb6))
* ignore verification ssl errors if verifyServer=false ([#453](https://github.com/gonicus/gonnect/issues/453)) ([be3d71c](https://github.com/gonicus/gonnect/commit/be3d71c419b252e4a103d5107ab9a9598b474742))
* make MWI subscription configurable ([#414](https://github.com/gonicus/gonnect/issues/414)) ([d74c530](https://github.com/gonicus/gonnect/commit/d74c53040e10f354cfbf86ef62662570af4bd3fc))
* make ReadOnlyConfdSettings treate string lists correctly ([#423](https://github.com/gonicus/gonnect/issues/423)) ([48328e2](https://github.com/gonicus/gonnect/commit/48328e235cf888f00b95b4b6792b727138aaf35c))
* more sip codes lead to auth request ([#379](https://github.com/gonicus/gonnect/issues/379)) ([31ee0cd](https://github.com/gonicus/gonnect/commit/31ee0cd138b21d34c3945274a4f6f08e344eae96))
* mwi recursions ([#405](https://github.com/gonicus/gonnect/issues/405)) ([2a42344](https://github.com/gonicus/gonnect/commit/2a423440dbafddb79db9a988e0bb27716e47ed1f))
* potential crash on HeadsetDevice destroyed ([#371](https://github.com/gonicus/gonnect/issues/371)) ([65900ab](https://github.com/gonicus/gonnect/commit/65900abcc5030f33687ebfab95e03f05c1591c51))
* prevent unhold on currently changing call ([#439](https://github.com/gonicus/gonnect/issues/439)) ([f1a05f5](https://github.com/gonicus/gonnect/commit/f1a05f59746755ebfb4d89def1b1bd1c8ff7a73a))
* raise window after showing it ([#398](https://github.com/gonicus/gonnect/issues/398)) ([4f72dc6](https://github.com/gonicus/gonnect/commit/4f72dc6ad54731be70190d1495c00b15b301c14e))
* reenable CardDAV plugin ([#406](https://github.com/gonicus/gonnect/issues/406)) ([9b9b136](https://github.com/gonicus/gonnect/commit/9b9b136f27fc945f2e6f6763401de76ead6a9a7b))
* resolve feeder manager deadlock ([#427](https://github.com/gonicus/gonnect/issues/427)) ([9a8d604](https://github.com/gonicus/gonnect/commit/9a8d6048e871262fda444a24a5091096940274f2))
* restore functionality for feeders without network requirements ([#387](https://github.com/gonicus/gonnect/issues/387)) ([01c6494](https://github.com/gonicus/gonnect/commit/01c64946a77c9cee40f566c9af2fecd51bfa7bcc))
* robustness for media and contacts cache ([#471](https://github.com/gonicus/gonnect/issues/471)) ([d879e1b](https://github.com/gonicus/gonnect/commit/d879e1b6a16ddca85658b20b8fb0fca63e51f05c))
* tab switch/reset notifications ([9338ed0](https://github.com/gonicus/gonnect/commit/9338ed0c6a82c35d44e36a40d90a66781c0c0441))
* **ui:** do not treat room names beginning with digits as phone numbers ([#461](https://github.com/gonicus/gonnect/issues/461)) ([75ed3f2](https://github.com/gonicus/gonnect/commit/75ed3f23ebe0730f10c43f6867d91cd1d54acf3f))
* **ui:** give focus to call screen when startet via enter ([#395](https://github.com/gonicus/gonnect/issues/395)) ([5e5d78e](https://github.com/gonicus/gonnect/commit/5e5d78e0a6364d511a43558e59be5ab020bf2795))
* use verifyServer flag for CA verification with webdav ([#437](https://github.com/gonicus/gonnect/issues/437)) ([c1ab98f](https://github.com/gonicus/gonnect/commit/c1ab98fe1294fc6fa62c204007f714f55d6eafe2))


### Features

* add Blinkstick busylight support ([#474](https://github.com/gonicus/gonnect/issues/474)) ([af400bd](https://github.com/gonicus/gonnect/commit/af400bd2b32bb19ff1e62c8ff9fc20531056d997))
* add template for 1&1 ([#465](https://github.com/gonicus/gonnect/issues/465)) ([865fcd9](https://github.com/gonicus/gonnect/commit/865fcd975f5d030674ff2d916c0eea5e1fa29456))
* adding global shortcuts support for windows ([#399](https://github.com/gonicus/gonnect/issues/399)) ([eb5237e](https://github.com/gonicus/gonnect/commit/eb5237ed507135d99f5d27b304eec25e0352df2a))
* basic MWI support ([#392](https://github.com/gonicus/gonnect/issues/392)) ([8c3c914](https://github.com/gonicus/gonnect/commit/8c3c91455b04a800a5c3e09c76b1f3e3b92ce023))
* delete old history entries ([#432](https://github.com/gonicus/gonnect/issues/432)) ([0ee826d](https://github.com/gonicus/gonnect/commit/0ee826d3384151954c559e3c417967f8b3fc872f))
* display sip registration status ([#375](https://github.com/gonicus/gonnect/issues/375)) ([f9e6567](https://github.com/gonicus/gonnect/commit/f9e6567a1bc89b5c688d7edb53fd4cc54899dcba))
* feeder plugin retry mechanism ([#391](https://github.com/gonicus/gonnect/issues/391)) ([e262f3f](https://github.com/gonicus/gonnect/commit/e262f3f75ef70ed54b1aca17c7169e06c7ebd94a))
* ical rrule parsing optimization ([#457](https://github.com/gonicus/gonnect/issues/457)) ([c46f066](https://github.com/gonicus/gonnect/commit/c46f066fd5f4f1aee410676f6e502ac682f4b265))
* **ldap:** configurable contact attribute mapping ([#445](https://github.com/gonicus/gonnect/issues/445)) ([4113384](https://github.com/gonicus/gonnect/commit/41133841f2c00c72229658a159abfae59330d7c9))
* pause/resume media on windows ([#370](https://github.com/gonicus/gonnect/issues/370)) ([61f3407](https://github.com/gonicus/gonnect/commit/61f34075a0f8dae316bd2a7ed6f6cf171917ad2b))
* plugins for microsoft calendars and contacts ([#428](https://github.com/gonicus/gonnect/issues/428)) ([e7e8cd9](https://github.com/gonicus/gonnect/commit/e7e8cd96bffc42e63dfb488894adb836c5ad4ff5))
* pre-fill missing it/es translations ([#377](https://github.com/gonicus/gonnect/issues/377)) ([4428e4e](https://github.com/gonicus/gonnect/commit/4428e4e659af0c708fa3738dc521977b7b11d1c2))
* rtt ui integration ([#443](https://github.com/gonicus/gonnect/issues/443)) ([62d48f3](https://github.com/gonicus/gonnect/commit/62d48f313a4aed5c971dc52d8e33a13e8396a58d))
* **ui:** leave conference via systray menu ([#438](https://github.com/gonicus/gonnect/issues/438)) ([b973bb6](https://github.com/gonicus/gonnect/commit/b973bb63c7798bea7fc6bf0c249e053c5d331fef))
* update pjproject to 2.17 ([#435](https://github.com/gonicus/gonnect/issues/435)) ([98c7199](https://github.com/gonicus/gonnect/commit/98c719971d48f696f312a21dde469c745c4ba220))
* windows signing ([#462](https://github.com/gonicus/gonnect/issues/462)) ([f7dd63a](https://github.com/gonicus/gonnect/commit/f7dd63a69d2231c92c768154eae99326a7ba1c29))

# [2.2.0-beta.9](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.8...v2.2.0-beta.9) (2026-05-28)


### Bug Fixes

* clear out URI encoded number input ([#475](https://github.com/gonicus/gonnect/issues/475)) ([cd1c126](https://github.com/gonicus/gonnect/commit/cd1c126b55aef57532db88ae432705d8421f7fe7))


### Features

* add Blinkstick busylight support ([#474](https://github.com/gonicus/gonnect/issues/474)) ([af400bd](https://github.com/gonicus/gonnect/commit/af400bd2b32bb19ff1e62c8ff9fc20531056d997))

# [2.2.0-beta.8](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.7...v2.2.0-beta.8) (2026-05-21)


### Bug Fixes

* robustness for media and contacts cache ([#471](https://github.com/gonicus/gonnect/issues/471)) ([d879e1b](https://github.com/gonicus/gonnect/commit/d879e1b6a16ddca85658b20b8fb0fca63e51f05c))

# [2.2.0-beta.7](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.6...v2.2.0-beta.7) (2026-05-20)


### Bug Fixes

* do not show favorites in "most called" list ([#456](https://github.com/gonicus/gonnect/issues/456)) ([8ee676d](https://github.com/gonicus/gonnect/commit/8ee676d08a6e79eba5c31749fb62531dfa6c7069))
* Ensure correct `tag` value type in flatpak manifest ([#466](https://github.com/gonicus/gonnect/issues/466)) ([3c5412a](https://github.com/gonicus/gonnect/commit/3c5412a250d5a7e3c3404432ae57d28465bac48c))
* **ui:** do not treat room names beginning with digits as phone numbers ([#461](https://github.com/gonicus/gonnect/issues/461)) ([75ed3f2](https://github.com/gonicus/gonnect/commit/75ed3f23ebe0730f10c43f6867d91cd1d54acf3f))


### Features

* add template for 1&1 ([#465](https://github.com/gonicus/gonnect/issues/465)) ([865fcd9](https://github.com/gonicus/gonnect/commit/865fcd975f5d030674ff2d916c0eea5e1fa29456))
* ical rrule parsing optimization ([#457](https://github.com/gonicus/gonnect/issues/457)) ([c46f066](https://github.com/gonicus/gonnect/commit/c46f066fd5f4f1aee410676f6e502ac682f4b265))
* rtt ui integration ([#443](https://github.com/gonicus/gonnect/issues/443)) ([62d48f3](https://github.com/gonicus/gonnect/commit/62d48f313a4aed5c971dc52d8e33a13e8396a58d))
* windows signing ([#462](https://github.com/gonicus/gonnect/issues/462)) ([f7dd63a](https://github.com/gonicus/gonnect/commit/f7dd63a69d2231c92c768154eae99326a7ba1c29))

# [2.2.0-beta.6](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.5...v2.2.0-beta.6) (2026-05-11)


### Bug Fixes

* base edits on SSL default configuration ([#459](https://github.com/gonicus/gonnect/issues/459)) ([6dcbf78](https://github.com/gonicus/gonnect/commit/6dcbf78a5b1b470ec966d8704b68e7d3e21da490))

# [2.2.0-beta.5](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.4...v2.2.0-beta.5) (2026-05-06)


### Bug Fixes

* if so configured, do not verify CA even if they are state ([#455](https://github.com/gonicus/gonnect/issues/455)) ([f45a1d8](https://github.com/gonicus/gonnect/commit/f45a1d81f152bb8d2024dad675206302255e2126))
* ignore verification ssl errors if verifyServer=false ([#453](https://github.com/gonicus/gonnect/issues/453)) ([be3d71c](https://github.com/gonicus/gonnect/commit/be3d71c419b252e4a103d5107ab9a9598b474742))

# [2.2.0-beta.5](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.4...v2.2.0-beta.5) (2026-05-05)


### Bug Fixes

* ignore verification ssl errors if verifyServer=false ([#453](https://github.com/gonicus/gonnect/issues/453)) ([be3d71c](https://github.com/gonicus/gonnect/commit/be3d71c419b252e4a103d5107ab9a9598b474742))

# [2.2.0-beta.4](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.3...v2.2.0-beta.4) (2026-05-04)


### Bug Fixes

* do not wipe all contact sources on by-source removals ([#450](https://github.com/gonicus/gonnect/issues/450)) ([f89882f](https://github.com/gonicus/gonnect/commit/f89882f9be298c1f90de881239b44a50b749492b))
* ignore certain ssl errors when verifyServer is false (webdav) ([#451](https://github.com/gonicus/gonnect/issues/451)) ([428860a](https://github.com/gonicus/gonnect/commit/428860a320785b9a52888a824d28990f59a6beb6))


### Features

* **ldap:** configurable contact attribute mapping ([#445](https://github.com/gonicus/gonnect/issues/445)) ([4113384](https://github.com/gonicus/gonnect/commit/41133841f2c00c72229658a159abfae59330d7c9))

# [2.2.0-beta.3](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.2...v2.2.0-beta.3) (2026-04-27)


### Bug Fixes

* prevent unhold on currently changing call ([#439](https://github.com/gonicus/gonnect/issues/439)) ([f1a05f5](https://github.com/gonicus/gonnect/commit/f1a05f59746755ebfb4d89def1b1bd1c8ff7a73a))

# [2.2.0-beta.2](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.1...v2.2.0-beta.2) (2026-04-23)


### Bug Fixes

* use verifyServer flag for CA verification with webdav ([#437](https://github.com/gonicus/gonnect/issues/437)) ([c1ab98f](https://github.com/gonicus/gonnect/commit/c1ab98fe1294fc6fa62c204007f714f55d6eafe2))


### Features

* **ui:** leave conference via systray menu ([#438](https://github.com/gonicus/gonnect/issues/438)) ([b973bb6](https://github.com/gonicus/gonnect/commit/b973bb63c7798bea7fc6bf0c249e053c5d331fef))

# [2.2.0-beta.2](https://github.com/gonicus/gonnect/compare/v2.2.0-beta.1...v2.2.0-beta.2) (2026-04-23)


### Bug Fixes

* use verifyServer flag for CA verification with webdav ([#437](https://github.com/gonicus/gonnect/issues/437)) ([c1ab98f](https://github.com/gonicus/gonnect/commit/c1ab98fe1294fc6fa62c204007f714f55d6eafe2))

# [2.2.0-beta.1](https://github.com/gonicus/gonnect/compare/v2.1.0...v2.2.0-beta.1) (2026-04-22)


### Bug Fixes

* build with certificate patch ([#433](https://github.com/gonicus/gonnect/issues/433)) ([8e8ba1f](https://github.com/gonicus/gonnect/commit/8e8ba1f9897457a4582ffdb059985569ac957984))
* c++ standard and guarding updates ([#422](https://github.com/gonicus/gonnect/issues/422)) ([9e8171b](https://github.com/gonicus/gonnect/commit/9e8171ba4ef5336e988d6938fa62793432d89ae5))
* code review loop ([#374](https://github.com/gonicus/gonnect/issues/374)) ([f1c609d](https://github.com/gonicus/gonnect/commit/f1c609da8dc0214e2f0c4909ef7b645b08352c11))
* disable WMME in pjsip ([#397](https://github.com/gonicus/gonnect/issues/397)) ([a90a80c](https://github.com/gonicus/gonnect/commit/a90a80cb4796c96f1626f82d3e59d49312c23585))
* handle direct pjsip mute again ([#402](https://github.com/gonicus/gonnect/issues/402)) ([0c4609c](https://github.com/gonicus/gonnect/commit/0c4609ca355f760478a19bda2c618aa299ff86eb))
* home/call switch tab notification reset ([#404](https://github.com/gonicus/gonnect/issues/404)) ([95e0a3f](https://github.com/gonicus/gonnect/commit/95e0a3ff849fda5a6fbb7e2294a99349edef77f8))
* make MWI subscription configurable ([#414](https://github.com/gonicus/gonnect/issues/414)) ([d74c530](https://github.com/gonicus/gonnect/commit/d74c53040e10f354cfbf86ef62662570af4bd3fc))
* make ReadOnlyConfdSettings treate string lists correctly ([#423](https://github.com/gonicus/gonnect/issues/423)) ([48328e2](https://github.com/gonicus/gonnect/commit/48328e235cf888f00b95b4b6792b727138aaf35c))
* more sip codes lead to auth request ([#379](https://github.com/gonicus/gonnect/issues/379)) ([31ee0cd](https://github.com/gonicus/gonnect/commit/31ee0cd138b21d34c3945274a4f6f08e344eae96))
* mwi recursions ([#405](https://github.com/gonicus/gonnect/issues/405)) ([2a42344](https://github.com/gonicus/gonnect/commit/2a423440dbafddb79db9a988e0bb27716e47ed1f))
* potential crash on HeadsetDevice destroyed ([#371](https://github.com/gonicus/gonnect/issues/371)) ([65900ab](https://github.com/gonicus/gonnect/commit/65900abcc5030f33687ebfab95e03f05c1591c51))
* raise window after showing it ([#398](https://github.com/gonicus/gonnect/issues/398)) ([4f72dc6](https://github.com/gonicus/gonnect/commit/4f72dc6ad54731be70190d1495c00b15b301c14e))
* reenable CardDAV plugin ([#406](https://github.com/gonicus/gonnect/issues/406)) ([9b9b136](https://github.com/gonicus/gonnect/commit/9b9b136f27fc945f2e6f6763401de76ead6a9a7b))
* resolve feeder manager deadlock ([#427](https://github.com/gonicus/gonnect/issues/427)) ([9a8d604](https://github.com/gonicus/gonnect/commit/9a8d6048e871262fda444a24a5091096940274f2))
* restore functionality for feeders without network requirements ([#387](https://github.com/gonicus/gonnect/issues/387)) ([01c6494](https://github.com/gonicus/gonnect/commit/01c64946a77c9cee40f566c9af2fecd51bfa7bcc))
* tab switch/reset notifications ([9338ed0](https://github.com/gonicus/gonnect/commit/9338ed0c6a82c35d44e36a40d90a66781c0c0441))
* **ui:** give focus to call screen when startet via enter ([#395](https://github.com/gonicus/gonnect/issues/395)) ([5e5d78e](https://github.com/gonicus/gonnect/commit/5e5d78e0a6364d511a43558e59be5ab020bf2795))


### Features

* adding global shortcuts support for windows ([#399](https://github.com/gonicus/gonnect/issues/399)) ([eb5237e](https://github.com/gonicus/gonnect/commit/eb5237ed507135d99f5d27b304eec25e0352df2a))
* basic MWI support ([#392](https://github.com/gonicus/gonnect/issues/392)) ([8c3c914](https://github.com/gonicus/gonnect/commit/8c3c91455b04a800a5c3e09c76b1f3e3b92ce023))
* delete old history entries ([#432](https://github.com/gonicus/gonnect/issues/432)) ([0ee826d](https://github.com/gonicus/gonnect/commit/0ee826d3384151954c559e3c417967f8b3fc872f))
* display sip registration status ([#375](https://github.com/gonicus/gonnect/issues/375)) ([f9e6567](https://github.com/gonicus/gonnect/commit/f9e6567a1bc89b5c688d7edb53fd4cc54899dcba))
* feeder plugin retry mechanism ([#391](https://github.com/gonicus/gonnect/issues/391)) ([e262f3f](https://github.com/gonicus/gonnect/commit/e262f3f75ef70ed54b1aca17c7169e06c7ebd94a))
* pause/resume media on windows ([#370](https://github.com/gonicus/gonnect/issues/370)) ([61f3407](https://github.com/gonicus/gonnect/commit/61f34075a0f8dae316bd2a7ed6f6cf171917ad2b))
* plugins for microsoft calendars and contacts ([#428](https://github.com/gonicus/gonnect/issues/428)) ([e7e8cd9](https://github.com/gonicus/gonnect/commit/e7e8cd96bffc42e63dfb488894adb836c5ad4ff5))
* pre-fill missing it/es translations ([#377](https://github.com/gonicus/gonnect/issues/377)) ([4428e4e](https://github.com/gonicus/gonnect/commit/4428e4e659af0c708fa3738dc521977b7b11d1c2))
* update pjproject to 2.17 ([#435](https://github.com/gonicus/gonnect/issues/435)) ([98c7199](https://github.com/gonicus/gonnect/commit/98c719971d48f696f312a21dde469c745c4ba220))

# [2.1.0](https://github.com/gonicus/gonnect/compare/v2.0.9...v2.1.0) (2026-04-10)


### Bug Fixes

* add missing export for CSV addressbook ([3fd2e92](https://github.com/gonicus/gonnect/commit/3fd2e92f61b77d4acd018521a12c2820ac050e89))
* always parse UTC offset / timezone ([#329](https://github.com/gonicus/gonnect/issues/329)) ([c74e7d5](https://github.com/gonicus/gonnect/commit/c74e7d53238669fbb3f46c18fb99267ca7d8e315))
* avoid early EDS objects-* signal triggers prior to view completion ([#335](https://github.com/gonicus/gonnect/issues/335)) ([555120a](https://github.com/gonicus/gonnect/commit/555120a71c715eedf8dbe06d9ce35e747c2ddfe9))
* avoid local time conversion for unknown timezones ([#333](https://github.com/gonicus/gonnect/issues/333)) ([bb45139](https://github.com/gonicus/gonnect/commit/bb45139b1debf06ffc7ca689a8afea2097e1f7fb))
* code review loop ([#374](https://github.com/gonicus/gonnect/issues/374)) ([60bbbc0](https://github.com/gonicus/gonnect/commit/60bbbc0f0e70fa163aae1fadf88cf72f23ef174e))
* default action on linux for desktop notifications ([#364](https://github.com/gonicus/gonnect/issues/364)) ([af682ef](https://github.com/gonicus/gonnect/commit/af682ef4f62dc2573c2a1ec535f6ff650dd93542))
* default setting for mute system forwarding ([#293](https://github.com/gonicus/gonnect/issues/293)) ([4a90bd5](https://github.com/gonicus/gonnect/commit/4a90bd5656590f70c106badd78946137c97ac9c0))
* **deps:** update EDS to 3.59.1 ([#285](https://github.com/gonicus/gonnect/issues/285)) ([8e3297b](https://github.com/gonicus/gonnect/commit/8e3297b0eb40988bc0bd8d54f7aeed0d765662bb))
* disable Real Time Text (RFC 4103) ([#345](https://github.com/gonicus/gonnect/issues/345)) ([01cf2d3](https://github.com/gonicus/gonnect/commit/01cf2d3684acb4f39f8b860fc03c3e9c1cc00d6e))
* disable WMME in pjsip ([#397](https://github.com/gonicus/gonnect/issues/397)) ([b08c87e](https://github.com/gonicus/gonnect/commit/b08c87ea32926e5f0c4185b7ef43f66a1f47a7df))
* do not call ldap feeder twice to prevent crash ([#348](https://github.com/gonicus/gonnect/issues/348)) ([fd305c6](https://github.com/gonicus/gonnect/commit/fd305c6775292a197d09a4d91fea51a128acaa67))
* first aid menu improvements ([#283](https://github.com/gonicus/gonnect/issues/283)) ([4680755](https://github.com/gonicus/gonnect/commit/46807554fe6aeb3535d306900c51acf4bb8d7242))
* fix occasional crashes when calls end on windows ([e10a37a](https://github.com/gonicus/gonnect/commit/e10a37a28453b9d58f6a6c3ec11a6caf3d2ac5f8))
* fix some wintoastlib related crashes ([#356](https://github.com/gonicus/gonnect/issues/356)) ([7ed606a](https://github.com/gonicus/gonnect/commit/7ed606a616193e917326e85ee9c4e3baa55b793a))
* force change for new beta release after rebase ([60ee872](https://github.com/gonicus/gonnect/commit/60ee872d0204c8d2b16d1ab7b29f82b511f6e5e2))
* handle direct pjsip mute again ([#402](https://github.com/gonicus/gonnect/issues/402)) ([cc9373f](https://github.com/gonicus/gonnect/commit/cc9373fce3d29a9939b5605e9a5d7af197aea5da))
* init Credentials class in flatpak if no SecretPortal ([#322](https://github.com/gonicus/gonnect/issues/322)) ([cb11ef2](https://github.com/gonicus/gonnect/commit/cb11ef20c94a18e4a1c2a178afa79d009f17f330))
* limit active ui edit mode dialogs to one & close on exiting edit mode ([#309](https://github.com/gonicus/gonnect/issues/309)) ([f2904ad](https://github.com/gonicus/gonnect/commit/f2904ad673adba9f4a6694907df244084ce48c9c))
* local shortcuts and help dialog ([#230](https://github.com/gonicus/gonnect/issues/230)) ([55e2133](https://github.com/gonicus/gonnect/commit/55e213300b94cde1a222c8174049661b7dad11ff))
* make flatpak build more stable ([07de32d](https://github.com/gonicus/gonnect/commit/07de32d5e3e4ba4b478a1096f2967ab199d6137f))
* make MWI subscription configurable ([#414](https://github.com/gonicus/gonnect/issues/414)) ([afc06fc](https://github.com/gonicus/gonnect/commit/afc06fcc75a60ce706e81d66dbd525b072d78fa0))
* match any/ no spaces around codec comma separator ([#368](https://github.com/gonicus/gonnect/issues/368)) ([4d3a981](https://github.com/gonicus/gonnect/commit/4d3a9813e75974d71d7197ebf5c803575725da86))
* more sip codes lead to auth request ([#379](https://github.com/gonicus/gonnect/issues/379)) ([4d53b22](https://github.com/gonicus/gonnect/commit/4d53b221e161f52ed946beed0683ce59aec22cfa))
* mwi recursions ([#405](https://github.com/gonicus/gonnect/issues/405)) ([a7b6e8a](https://github.com/gonicus/gonnect/commit/a7b6e8a303e03208d07b2f7aebb2437147ed6d5e))
* optimize search for pure numbers ([0201a25](https://github.com/gonicus/gonnect/commit/0201a259bbcbfe2e38ac2034b9c76644d035879f))
* platform updates ([#259](https://github.com/gonicus/gonnect/issues/259)) ([84d515e](https://github.com/gonicus/gonnect/commit/84d515e61d2467ff2b8f3b450c3591e110da09b0))
* potential crash on HeadsetDevice destroyed ([#371](https://github.com/gonicus/gonnect/issues/371)) ([ce9045b](https://github.com/gonicus/gonnect/commit/ce9045be09af4b20545f8edd921d7af0eb8690a5))
* prevent entering of invalid DTMF chars to prevent crash ([#287](https://github.com/gonicus/gonnect/issues/287)) ([42cb3df](https://github.com/gonicus/gonnect/commit/42cb3dfd548c8a607db20c3f473f62558e051132))
* prevent hold of ongoing call on incoming when busyOnBusy ([#297](https://github.com/gonicus/gonnect/issues/297)) ([e061657](https://github.com/gonicus/gonnect/commit/e0616576501dfe01cc961345be2505b4674b9b22))
* raise window after showing it ([#398](https://github.com/gonicus/gonnect/issues/398)) ([6da44fa](https://github.com/gonicus/gonnect/commit/6da44fa478a0bdd8871708fd0d4071f309b4c4eb))
* reenable CardDAV plugin ([#406](https://github.com/gonicus/gonnect/issues/406)) ([6829e25](https://github.com/gonicus/gonnect/commit/6829e2578310e529413415cf306c4fe2cf5cdcee))
* refactor credential store ([#363](https://github.com/gonicus/gonnect/issues/363)) ([9148e51](https://github.com/gonicus/gonnect/commit/9148e51f947469886cc97a9b496709ed6911a275))
* remove custom libusb building ([658daa1](https://github.com/gonicus/gonnect/commit/658daa1601ff10be0b4959c7399e074b3b9de601))
* restore functionality for feeders without network requirements ([#387](https://github.com/gonicus/gonnect/issues/387)) ([ce6639c](https://github.com/gonicus/gonnect/commit/ce6639cb364873a5ea0484b0c7c7409ea24465bf))
* tab creation failure due to new missing boolean ([#347](https://github.com/gonicus/gonnect/issues/347)) ([ac48dbf](https://github.com/gonicus/gonnect/commit/ac48dbf92e1f4c959fe688ce94fb34bcd399f3b6))
* tab switch/reset notifications ([5cdaf86](https://github.com/gonicus/gonnect/commit/5cdaf868ed4f8c3041a44f7f5134c9ce0133e6a1))
* **ui:** copy full url to clipboard not just room name (DateEventsList) ([#218](https://github.com/gonicus/gonnect/issues/218)) ([9452bfa](https://github.com/gonicus/gonnect/commit/9452bfada7177d4842fd8afc40f3470a48654f21))
* **ui:** do not disable history items in search list (if number) ([#314](https://github.com/gonicus/gonnect/issues/314)) ([c44b1b2](https://github.com/gonicus/gonnect/commit/c44b1b20bc56d88599c6fd05cf8df901b294b0fe)), closes [#311](https://github.com/gonicus/gonnect/issues/311)
* **ui:** fixed selection behaviour in call list ([#271](https://github.com/gonicus/gonnect/issues/271)) ([734c9d8](https://github.com/gonicus/gonnect/commit/734c9d84677facc8d1dc3d345ba1fb3d0008dc66))
* **ui:** give focus to call screen when startet via enter ([#395](https://github.com/gonicus/gonnect/issues/395)) ([494402b](https://github.com/gonicus/gonnect/commit/494402b3315c0974beb8bb529bde332884a5ddfb))
* **ui:** jitsi meet chat now cleared when leaving room ([#239](https://github.com/gonicus/gonnect/issues/239)) ([50c6bc5](https://github.com/gonicus/gonnect/commit/50c6bc51d2e700c9f5c0369d0aa13239642fa00b))
* **ui:** loading of own avatar ([#290](https://github.com/gonicus/gonnect/issues/290)) ([f11c4bc](https://github.com/gonicus/gonnect/commit/f11c4bc0817e06c34cdbbc75b044bf8d0c799346)), closes [#276](https://github.com/gonicus/gonnect/issues/276)
* **ui:** next button in template wizard never enabled ([#291](https://github.com/gonicus/gonnect/issues/291)) ([3107665](https://github.com/gonicus/gonnect/commit/310766560bd0fa8f495524e63261f3bab85d2955))
* **ui:** show name/time labels in jitis chat again ([#346](https://github.com/gonicus/gonnect/issues/346)) ([d60c925](https://github.com/gonicus/gonnect/commit/d60c925f51592b3d9e9e1608b11def1f625858d2))
* unhold other call when starting conference ([#286](https://github.com/gonicus/gonnect/issues/286)) ([33c43ec](https://github.com/gonicus/gonnect/commit/33c43ec6f07a9b76e4e15e33ccbeafb8fa41d49a))
* use global ro settings for sip section ([#352](https://github.com/gonicus/gonnect/issues/352)) ([fe51b0e](https://github.com/gonicus/gonnect/commit/fe51b0e36cf7597c3d8c7b45262d560300f18382))
* windows block logout / shutdown on ongoing call ([#338](https://github.com/gonicus/gonnect/issues/338)) ([bc1f790](https://github.com/gonicus/gonnect/commit/bc1f7909f7cc368ea8fbd0e2c8a0ea57e7d37494))
* windows block logout / shutdown on ongoing call ([#338](https://github.com/gonicus/gonnect/issues/338)) ([54e4657](https://github.com/gonicus/gonnect/commit/54e4657a68c1b0fe2fd31b33f18be82a7a886d67))


### Features

* add config-based codec selection ([#354](https://github.com/gonicus/gonnect/issues/354)) ([b44acf2](https://github.com/gonicus/gonnect/commit/b44acf29cdf6ba35fa042b12809eeffbe7e59b7e))
* add inhibit to windows ([#280](https://github.com/gonicus/gonnect/issues/280)) ([6e67f93](https://github.com/gonicus/gonnect/commit/6e67f938c830738767d5f691084a4e0b2bbd0f3d))
* add Qt QML  accessibility features to the UI ([#351](https://github.com/gonicus/gonnect/issues/351)) ([aff8468](https://github.com/gonicus/gonnect/commit/aff8468ab372d6a51046d093600a29f58c78f181))
* allow pre-processing of search strings ([#274](https://github.com/gonicus/gonnect/issues/274)) ([ac104e4](https://github.com/gonicus/gonnect/commit/ac104e40c8b30d4a0fba747e400c2d4c2044e768))
* basic MWI support ([#392](https://github.com/gonicus/gonnect/issues/392)) ([dcb7fbc](https://github.com/gonicus/gonnect/commit/dcb7fbcc9100e113f219afaf038318283bf5393a))
* basic RTT handling in call ([#355](https://github.com/gonicus/gonnect/issues/355)) ([abbccfa](https://github.com/gonicus/gonnect/commit/abbccfa527965bca5cd1632f381747829e6c9896))
* blocking address sources ([#325](https://github.com/gonicus/gonnect/issues/325)) ([ac61c48](https://github.com/gonicus/gonnect/commit/ac61c4840d2321cc1a8f5105ce9e01df303bd1d8)), closes [#312](https://github.com/gonicus/gonnect/issues/312)
* calendar expansion ([#250](https://github.com/gonicus/gonnect/issues/250)) ([9cfdb2d](https://github.com/gonicus/gonnect/commit/9cfdb2d9d52b1c85e846c1f1c55b769679f198e2))
* configure jitsi MCU via config ([#337](https://github.com/gonicus/gonnect/issues/337)) ([522de41](https://github.com/gonicus/gonnect/commit/522de41809fca9ad74d78eadf87331a617933e50))
* display sip registration status ([#375](https://github.com/gonicus/gonnect/issues/375)) ([4bf60d0](https://github.com/gonicus/gonnect/commit/4bf60d0af998ce1d773bc60f892b31252b30f871))
* do not send notifications for conferences/appointments that have been joined early ([#298](https://github.com/gonicus/gonnect/issues/298)) ([91ce62e](https://github.com/gonicus/gonnect/commit/91ce62e9613c5675d2d26fc5eaebc6afc9245f11))
* dynamic widget pages/dashboards ([#251](https://github.com/gonicus/gonnect/issues/251)) ([829952b](https://github.com/gonicus/gonnect/commit/829952b9a0389de1f3a6227f9d7aa76770fce430))
* improved handling of full- and multi-day conferences/appointments ([#302](https://github.com/gonicus/gonnect/issues/302)) ([e55f897](https://github.com/gonicus/gonnect/commit/e55f89735e9e1a0e2a77f8827cab7af34c2c8022))
* introduce custom widget settings and a new web display widget ([#284](https://github.com/gonicus/gonnect/issues/284)) ([30055e5](https://github.com/gonicus/gonnect/commit/30055e5edb007a7463fb0fc628519e1cb74a5ab2))
* packaging tuner ([#261](https://github.com/gonicus/gonnect/issues/261)) ([6bc1792](https://github.com/gonicus/gonnect/commit/6bc1792455d8c67524e99259d953536cc904a219))
* pause/resume media on windows ([#370](https://github.com/gonicus/gonnect/issues/370)) ([cf7dd0f](https://github.com/gonicus/gonnect/commit/cf7dd0f1355f537ae878efe8927cfd1c5fcc93b2))
* pre-fill missing it/es translations ([#377](https://github.com/gonicus/gonnect/issues/377)) ([6c78b55](https://github.com/gonicus/gonnect/commit/6c78b55fa6612d630b201382a2d88b122fe2cf36))
* retrieve and show phone numbers for joining jitsi conference ([#334](https://github.com/gonicus/gonnect/issues/334)) ([ec2804c](https://github.com/gonicus/gonnect/commit/ec2804c01736e9b82e621626a2bfc120d9ec5f4f))
* show connection quality and security details ([#341](https://github.com/gonicus/gonnect/issues/341)) ([83734f0](https://github.com/gonicus/gonnect/commit/83734f081b20a97abb2c3f5b35d03bd5c246fbf8))
* support more platforms ([#233](https://github.com/gonicus/gonnect/issues/233)) ([19fff59](https://github.com/gonicus/gonnect/commit/19fff592caead967c73517deed84d01d4dfbe690))
* toggle whiteboard in jitsi meet ([#235](https://github.com/gonicus/gonnect/issues/235)) ([23df6a2](https://github.com/gonicus/gonnect/commit/23df6a2a11fa3f1eff1316ddd1116ec5e6a8ccf2))
* **ui:** automatically select first search item in global search ([#292](https://github.com/gonicus/gonnect/issues/292)) ([117945a](https://github.com/gonicus/gonnect/commit/117945a6dbfb5b398459117ec87d03c05d571e0d))
* **ui:** show red dot on active main tab bar button ([#326](https://github.com/gonicus/gonnect/issues/326)) ([676f283](https://github.com/gonicus/gonnect/commit/676f2836aeedb7f2c34a82a82f280943b2fd6073))


### Reverts

* Revert "chore(release): 2.1.0-beta.4 [skip ci]" ([e9e4cad](https://github.com/gonicus/gonnect/commit/e9e4cad491c7999cb245d326962bb3f0fc9ed841))

# [2.1.0-beta.8](https://github.com/gonicus/gonnect/compare/v2.1.0-beta.7...v2.1.0-beta.8) (2026-04-01)


### Bug Fixes

* make MWI subscription configurable ([#414](https://github.com/gonicus/gonnect/issues/414)) ([afc06fc](https://github.com/gonicus/gonnect/commit/afc06fcc75a60ce706e81d66dbd525b072d78fa0))

# [2.1.0-beta.7](https://github.com/gonicus/gonnect/compare/v2.1.0-beta.6...v2.1.0-beta.7) (2026-03-30)


### Bug Fixes

* disable WMME in pjsip ([#397](https://github.com/gonicus/gonnect/issues/397)) ([b08c87e](https://github.com/gonicus/gonnect/commit/b08c87ea32926e5f0c4185b7ef43f66a1f47a7df))
* handle direct pjsip mute again ([#402](https://github.com/gonicus/gonnect/issues/402)) ([cc9373f](https://github.com/gonicus/gonnect/commit/cc9373fce3d29a9939b5605e9a5d7af197aea5da))
* mwi recursions ([#405](https://github.com/gonicus/gonnect/issues/405)) ([a7b6e8a](https://github.com/gonicus/gonnect/commit/a7b6e8a303e03208d07b2f7aebb2437147ed6d5e))
* raise window after showing it ([#398](https://github.com/gonicus/gonnect/issues/398)) ([6da44fa](https://github.com/gonicus/gonnect/commit/6da44fa478a0bdd8871708fd0d4071f309b4c4eb))
* reenable CardDAV plugin ([#406](https://github.com/gonicus/gonnect/issues/406)) ([6829e25](https://github.com/gonicus/gonnect/commit/6829e2578310e529413415cf306c4fe2cf5cdcee))
* restore functionality for feeders without network requirements ([#387](https://github.com/gonicus/gonnect/issues/387)) ([ce6639c](https://github.com/gonicus/gonnect/commit/ce6639cb364873a5ea0484b0c7c7409ea24465bf))
* tab switch/reset notifications ([5cdaf86](https://github.com/gonicus/gonnect/commit/5cdaf868ed4f8c3041a44f7f5134c9ce0133e6a1))
* **ui:** give focus to call screen when startet via enter ([#395](https://github.com/gonicus/gonnect/issues/395)) ([494402b](https://github.com/gonicus/gonnect/commit/494402b3315c0974beb8bb529bde332884a5ddfb))


### Features

* basic MWI support ([#392](https://github.com/gonicus/gonnect/issues/392)) ([dcb7fbc](https://github.com/gonicus/gonnect/commit/dcb7fbcc9100e113f219afaf038318283bf5393a))
* display sip registration status ([#375](https://github.com/gonicus/gonnect/issues/375)) ([4bf60d0](https://github.com/gonicus/gonnect/commit/4bf60d0af998ce1d773bc60f892b31252b30f871))

# [2.1.0-beta.6](https://github.com/gonicus/gonnect/compare/v2.1.0-beta.5...v2.1.0-beta.6) (2026-03-11)


### Bug Fixes

* code review loop ([#374](https://github.com/gonicus/gonnect/issues/374)) ([60bbbc0](https://github.com/gonicus/gonnect/commit/60bbbc0f0e70fa163aae1fadf88cf72f23ef174e))
* more sip codes lead to auth request ([#379](https://github.com/gonicus/gonnect/issues/379)) ([4d53b22](https://github.com/gonicus/gonnect/commit/4d53b221e161f52ed946beed0683ce59aec22cfa))
* potential crash on HeadsetDevice destroyed ([#371](https://github.com/gonicus/gonnect/issues/371)) ([ce9045b](https://github.com/gonicus/gonnect/commit/ce9045be09af4b20545f8edd921d7af0eb8690a5))


### Features

* pause/resume media on windows ([#370](https://github.com/gonicus/gonnect/issues/370)) ([cf7dd0f](https://github.com/gonicus/gonnect/commit/cf7dd0f1355f537ae878efe8927cfd1c5fcc93b2))
* pre-fill missing it/es translations ([#377](https://github.com/gonicus/gonnect/issues/377)) ([6c78b55](https://github.com/gonicus/gonnect/commit/6c78b55fa6612d630b201382a2d88b122fe2cf36))

# [2.1.0-beta.5](https://github.com/gonicus/gonnect/compare/v2.1.0-beta.4...v2.1.0-beta.5) (2026-03-05)


### Bug Fixes

* force change for new beta release after rebase ([60ee872](https://github.com/gonicus/gonnect/commit/60ee872d0204c8d2b16d1ab7b29f82b511f6e5e2))

# [2.1.0-beta.4](https://github.com/gonicus/gonnect/compare/v2.1.0-beta.3...v2.1.0-beta.4) (2026-02-23)


### Bug Fixes

* avoid early EDS objects-* signal triggers prior to view completion ([#335](https://github.com/gonicus/gonnect/issues/335)) ([d1c14e4](https://github.com/gonicus/gonnect/commit/d1c14e4b1abc367d9184ecbe413edd8232412c4a))
* disable Real Time Text (RFC 4103) ([#345](https://github.com/gonicus/gonnect/issues/345)) ([c50890c](https://github.com/gonicus/gonnect/commit/c50890c202491a4ee597a54b900e50e7793cab7d))
* do not call ldap feeder twice to prevent crash ([#348](https://github.com/gonicus/gonnect/issues/348)) ([0f51ab3](https://github.com/gonicus/gonnect/commit/0f51ab30c89e1d17fe361830cd822febdc51d75f))
* tab creation failure due to new missing boolean ([#347](https://github.com/gonicus/gonnect/issues/347)) ([518fecb](https://github.com/gonicus/gonnect/commit/518fecbd862fe1ce593b127a8cdef8f505c56007))
* **ui:** show name/time labels in jitis chat again ([#346](https://github.com/gonicus/gonnect/issues/346)) ([3c20c30](https://github.com/gonicus/gonnect/commit/3c20c3038c0cc5f55914f4886844892cad6f124b))
* use global ro settings for sip section ([#352](https://github.com/gonicus/gonnect/issues/352)) ([19dd636](https://github.com/gonicus/gonnect/commit/19dd6360bdc32a46d4fc0d97ec15e78c9f35daca))
* windows block logout / shutdown on ongoing call ([#338](https://github.com/gonicus/gonnect/issues/338)) ([06044c1](https://github.com/gonicus/gonnect/commit/06044c1afb8bc365079edc6af50dab2dd8ecf5ea))


### Features

* show connection quality and security details ([#341](https://github.com/gonicus/gonnect/issues/341)) ([8e5623e](https://github.com/gonicus/gonnect/commit/8e5623e7afcf0a14fb07f7f8f09324cce77d2432))

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
