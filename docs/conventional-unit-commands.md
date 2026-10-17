# Conventional Unit command panel

The bottom bar shows a command panel for the selected conventional Former or
Colony Pod. Former actions retain the actual keyboard bindings: Farm **F**,
Road **R**, Forest **O**, Solar **S**, Mine **M**, and Cancel **C**. Colony Pods
offer Found Base **B**. Only currently admitted actions appear. The Former
heading displays its saved order and raw stored work byte; it does not estimate
completion time or imply that cancellation refunds movement or stored work.

The detached Unit query reuses the existing work and founding admission paths.
An unavailable turn, paused research choice, foreign owner, or invalid site
offers no action. The panel refreshes on selection and relevant state events,
not render frames. A click checks the selected scalar Unit ID and freshly
queries admission before sending the existing authenticated native event.
Retired selection callbacks cannot command a replacement Unit. Legacy native
Units and conventional combat Units retain their existing bottom bar.

Validation: the standalone actual GSE panel callback test passes using DOM and
query doubles. Both changed production TUs and both changed native-test TUs
pass syntax checking; focused LLVM 15 analysis of the two production TUs reports
no diagnostics. The added actual-Former panel group and amended actual-Colony
founding group await the shared cached linked run. Those fixtures exercise
real wrappers/events, saved work/cancellation, stale selection/site callbacks,
read-only queries, and unchanged RNG. No platform build or rendered screenshot
is claimed here.

At the interactive checkpoint, capture the 1280×800 bottom bar for an idle
Former, an active order after work, cancellation with retained work, and a
Colony Pod on valid/invalid founding sites. Check keyboard/button consistency,
selection changes, research pause, and that the middle-area tabs remain usable.
