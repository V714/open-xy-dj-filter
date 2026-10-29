# DJ Filter extension

Use the adjacent Open-XY SDK. This extension needs its master-filter interface
(`sdk/master-processor-interface`, targeting upstream `staging`).

Keep DSP, coefficient generators, room geometry, artwork and sound tests here.
Keep firmware addresses, assembly, storage, menu and native drawing adapters in
the SDK. Contributor C++ uses no platform conditionals or OS headers.
Audio callbacks never allocate, block, perform I/O or call graphics.

Run `python ../open-xy/sdk.py test --extension .` with the matching SDK checkout.
Generate coefficients with `python master_filter_tables.py`; generate room SVG
and Q10 geometry with `python master_filter_artwork.py`. Commit generated source
and authored SVG together. Firmware and packages stay outside all checkouts.
Do not claim measured CPU or device acceptance from host tests.
