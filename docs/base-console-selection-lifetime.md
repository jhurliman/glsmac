# Base console notification ownership

Base selection and Base-card preview notifications are delivered through the
captured game session and a stable Base ID. They do not retain a frontend Base,
manager, local pointer reference or old backend wrapper. Selection publishes
Tile preview, Base preview and Base selection in that order, resolving the Base
again and checking the session after each listener boundary. A removed Base or
replaced session stops remaining selection notifications. Nullable card refresh
clears the selected card without dereferencing a missing Base in the roster.

The BasePreview presenter uses the same weak lifetime guard as UnitPreview, so
retained DOM update/remove callbacks cannot access the retired presenter.

The focused native fixture covers ordinary deferred delivery, retirement before
delivery, retirement from a preview listener, session replacement with the same
numeric Base ID, hover/card delivery, a null card and presenter retirement. It
uses actual frontend BaseManager/Game/Widget calls and backend Base registries.
The actual GSE roster caller also admits a null/undefined preview safely.

This slice fixes functional ownership; it does not establish native Base-card
portrait or population-badge geometry. The existing fitted Base mesh remains
until the separate original selected/roster Base painter contract is available.
The Scout card contract must not be extrapolated to Base portraits.
