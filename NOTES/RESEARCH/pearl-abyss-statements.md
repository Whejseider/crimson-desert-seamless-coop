# Pearl Abyss — Official Statements on Mods and Multiplayer/Co-op (+ BDO lineage)

Retrieved: 2026-10-31. **Caveat:** none of the news pages could be fetched directly from this
environment (kitguru, pcgamesn, mp1st, digitaltoday, crimsondesert.pearlabyss.com all unreachable;
web.archive.org refused). All quotes below come from **search-result snippets** and are the outlets'
English renderings (often translated from Korean). Treat exact wording as med confidence; verify against
the originals before quoting publicly.

## Key takeaways

- **Mods:** No official mod support, SDK, or tools. Position since launch: "discussing internally / reviewing broadly", **no decision**. Key reason given: mod tools "would require disclosing a significant portion of the engine" (CEO Heo Jin-young, AGM 2026-03-27). Pre-launch PR lead Will Powers: no official mod support at release, "we need to have those conversations". July 2026 AGM: still under review (CEO + COO Lee Dong-won). Confidence: med-high (consistent across many outlets).
- **No anti-mod enforcement found:** PA has not banned single-player modders and does not actively block mods (secondary source). No ToS text on modding for Crimson Desert found (only an old BDO thread). Confidence: low-med.
- **Multiplayer/co-op:** Shipped single-player only (2026-03-19). Will Powers pre-launch, asked if it would have multiplayer: "No" (single source). Since then executives repeatedly say multiplayer is **being studied, not committed**: Feb 12 2026 Q4-2025 call (post-launch multiplayer/DLC possible "depending on market demand", "similar to GTA"); July 7 2026 AGM (CEO: "there is room for a multiplayer mode"; many issues incl. **server structure**); Aug 13 2026 Q2 letter ("significant potential for expansion into multiplayer modes", no finalized plans). **No official co-op announced as of 2026-10-31.** Confidence: med-high.
- **Implication for our project:** an official co-op/multiplayer mode could arrive later (risk of overlap/obsolescence, and PA might then care more about network tampering). Currently no hostility to mods; PA's crash reporter sends crash reports to PA (cdmp README notes this).
- **BDO lineage:** Crimson Desert was revealed (G-Star / Nov 2019) as an MMORPG prequel to Black Desert Online, later reworked into single-player. Its exe still contains BDO-style client+server "Troc" networking running over an in-memory loopback — which is exactly what community co-op mods exploit.

## 1. Mod support — timeline

| Date | Speaker / venue | Statement (outlet rendering) | Source |
|---|---|---|---|
| Pre-launch (~early 2026) | Will Powers (PA PR/marketing lead), interview | No official mod support at launch; "we need to have those conversations"; "Historically, this has never even been a conversation at the studio." Would not rule it out; not a priority. | PCGamesN (snippet) |
| 2026-03-27 | CEO Heo Jin-young, annual general shareholders' meeting | "providing mod tools would require disclosing a significant portion of the engine, so there are no concrete plans yet"; mods seen as positive/"strong advantage" for a sandbox. | KitGuru, Insider Gaming, Otakukart, instant-gaming (snippets) |
| ~2026-05 | Reports around sales milestone | PA prioritising sales over mod support/DLC. | TweakTown (snippet) |
| 2026-07-07 | CEO Heo + COO Lee Dong-won, shareholder meeting | Interested in mods since before launch, not decided internally; COO: "reviewing it broadly" as the mod community became active. | MP1st, KitGuru, shazoo.ru (2026-07-10), Digital Today (snippets) |
| to 2026-10-31 | — | No later announcement found. | searched "Pearl Abyss Crimson Desert mod support statement" |

## 2. Multiplayer / co-op — timeline

| Date | Speaker / venue | Statement | Source |
|---|---|---|---|
| 2019 | Reveal | Announced as MMORPG prequel to BDO | Windows Central, thegameswiki (snippets) |
| Pre-launch | Will Powers | Asked if multiplayer: "No." | Broken Builds (snippet, single source) |
| 2026-02-12 | Q4 2025 earnings call | Multiplayer and DLC could come post-launch "depending on market demand"; described as "similar to GTA" (main story single-player, separate multiplayer mode) | allthings.how, digitalcitizen, Notebookcheck (snippets) |
| ~2026-03 | Wccftech interview | Multiplayer "unlikely" for the time being; Switch 2 R&D begun | Wccftech (snippet) |
| 2026-06 | Roadmap Jun–Sep | Cross-save, story, balance, major DLC — no multiplayer item | RPGamer, MP1st, Insider Gaming (snippets) |
| 2026-07-07 | CEO Heo, shareholder meeting | "While it is not the stage to discuss a confirmed release plan, we are continuously reviewing ways to allow users to enjoy the Crimson Desert IP for longer, and I believe there is room for a multiplayer mode in that process." Many issues to review incl. server structure. CEO also: "studying" technical feasibility and gameplay. | MP1st, KitGuru, Digital Today (snippets) |
| 2026-08-13 | Q2 2026 earnings letter | "significant potential for expansion into multiplayer modes" but no finalized plans; DLC year-end; Switch 2 H1 2027 | Push Square, massivelyop, consolepulse (snippets) |
| 2026-10-29 | Patch 2.04.00 + DLC "Charting the Unknown" | No multiplayer content in notes (sailing, diving, marriage, housing, castles) | keengamer, vulkk (snippets) |
| ~2026-10-06 | Community | Fan co-op mod test video (u/No-Context2224); PA has not commented (none found) | Notebookcheck, Insider Gaming, G2A, tech4gamers (snippets) |

## 3. Other relevant official positions
- Denuvo: PA said pre-release builds already used Denuvo, so reviewed performance included it (multiplayer.it snippet, March 2026).
- Privacy policy change notice (gamegpu snippet) — not investigated.
- Official site patch notices: https://crimsondesert.pearlabyss.com/en-US/News/Notice/Detail?_boardNo=128 (2.01.00), boardNo=123 (2.00.00) — URLs from search, not fetchable here.

## 4. Black Desert Online heritage (brief)
- **Engine:** BlackSpace = Pearl Abyss's in-house engine lineage from Black Desert (reworked); BDO also uses PAZ archives (BDO used **ICE** encryption on PAZ/`.loc`; Crimson Desert uses **ChaCha20** with filename-derived keys — different scheme, same container idea). (ByJunter troc-bdo-y-runtime.md; kukdh1/PAZ-Unpacker is BDO-only) — med-high.
- **Networking:** BDO's client protocol used `Tr…Req`/Ack/Nak message classes (heartbeat opcode 0x3E9, ~450 packet types, documented 2015–16 by emulator projects like Illuminate/DesertProject). Crimson Desert keeps the same Troc design (`pa::TrocTr*` handlers, RTTI namespace `pa::` vs BDO's `ge::`), now with ~1,115 handlers and the server compiled into the client process over a virtual session. Inherited "other player" messages (`UpdateVisiblePlayerAck`, `OtherEquipItemAck`, `MoveActorAck`, `RemoveActorAck`) and MMO remnants (auth/field server, party, guild, World Market, SQL procs). — med.
- **Protection:** BDO used Themida + XIGNCODE3 (2017 analysis); Crimson Desert uses Themida + Denuvo, no XIGNCODE found. — med.
- Systems resembling BDO: open-world horse/mount systems, crafting, housing/home building, "World Market" remnants, PA Action Chart (`.paac`) animation state machines (also a BDO concept) — low-med.

## Rumor / unverified
- "Similar to GTA" multiplayer framing — snippet-only.
- Will Powers' one-word "No" — single secondary source.
- Dexerto claim the game "already sees mod support" refers to community mods, not official.

## Impossibility claims
- None from Pearl Abyss stating mods or co-op are impossible. Closest: CEO says official mod tools are hard because they'd expose the engine (2026-03-27); reports that a broad online mode would be "far more difficult" (infrastructure, progression balancing, long-term support) (digitalcitizen/Notebookcheck snippet).

## Sources (retrieved 2026-10-31; all via search snippets unless noted)
- https://www.pcgamesn.com/crimson-desert/mod-support-pc — Will Powers mod remarks — med
- https://www.kitguru.net/gaming/matthew-wilson/pearl-abyss-still-exploring-mod-support-and-multiplayer-for-crimson-desert/ — July AGM — med
- https://www.kitguru.net/gaming/mustafa-mahmoud/crimson-desert-could-receive-mod-support-in-the-future-claims-pearl-abyss-ceo/ — March AGM — med
- https://insider-gaming.com/crimson-desert-developer-addresses-official-mod-support/ — med
- https://mp1st.com/news/crimson-desert-multiplayer-not-ruled-out-by-pearl-abyss-still-undecided-on-mod-support — July AGM quote — med
- https://www.digitaltoday.co.kr/en/view/79035/pearl-abyss-opens-shareholder-meeting-focus-turns-to-what-comes-after-crimson-desert — July 7 AGM, server structure — med
- https://otakukart.com/pearl-abyss-ceo-says-official-mod-support-for-crimson-desert-is-under-review-as-studio-explores-a-creator-ecosystem/ — med
- https://news.instant-gaming.com/en/articles/18761-there-are-no-plans-for-official-mod-support-for-crimson-desert — med
- https://shazoo.ru/2026/07/10/187017/pearl-abyss-rassmatrivaet-podderzku-modov-dlia-crimson-desert — med
- https://www.tweaktown.com/news/110703/pearl-abyss-more-interested-in-boosting-crimson-deserts-sales-than-making-dlc-or-mod-support-as-game-nears-5-million-sales/ — low-med
- https://brokenbuilds.gg/crimson-desert/guides/is-crimson-desert-multiplayer — Powers "No" — low-med
- https://allthings.how/crimson-desert-is-single-player-at-launch-but-co-op-is-planned-for-later/ — Feb call — med
- https://wccftech.com/crimson-desert-switch-2-rd-multiplayer-unlikely/ — med
- https://pushsquare.com/news/2026/08/crimson-desert-dlc-on-track-for-2026-release-as-dev-ponders-multiplayer-modes — Q2 letter — med
- https://massivelyop.com/2026/08/13/pearl-abyss-q2-2026-crimson-desert-tapers-off-black-desert-flattens-dokev-eyes-2028/ — med
- https://www.digitalcitizen.life/crimson-desert-may-add-multiplayer-later-as-pearl-abyss-continues-testing-a-switch-2-version/ — med
- https://www.notebookcheck.net/Could-Crimson-Desert-get-co-op-Early-tests-look-promising.1417136.0.html ; https://insider-gaming.com/crimson-desert-multiplayer-mod-is-being-tested/ ; https://tech4gamers.com/modders-crimson-desert-co-op/ — fan co-op mod — med
- https://www.dexerto.com/wikis/crimson-desert/does-crimson-desert-have-mod-support — no bans — low-med
- https://multiplayer.it/notizie/autori-crimson-desert-dicono-non-dobbiamo-preoccuparci-denuvo-problemi-prestazionali.html — Denuvo response — med
- https://www.windowscentral.com/crimson-desert-everything-you-need-know ; https://insider-gaming.com/?p=180450 — MMO origins — med
- https://github.com/ByJunter/CrimsonDesertMultiplayer/blob/main/docs/research/troc-bdo-y-runtime.md (fetched directly) — BDO vs CD Troc — med-high
