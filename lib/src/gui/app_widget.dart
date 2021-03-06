import 'package:auto_route/auto_route.dart';
import 'package:flex_color_scheme/flex_color_scheme.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:window_manager/window_manager.dart';

import '../app/pages.dart';
import '../app/route.gr.dart';
import '../core/notification_controller.dart';
import '../preference/storage_box.dart';
import '../preference/window_state.dart';
import '../state/notifier.dart';
import '../state/settings_state.dart';

final themeSettingProvider = StateNotifierProvider<ExclusiveItemsNotifier<ThemeMode>, ThemeMode>((ref) {
  final box = ref.watch(storageBoxProvider);
  return ExclusiveItemsNotifier<ThemeMode>(
    entry: StorageEntry(box: box, key: SettingsEntryKey.themeMode.name),
    values: [ThemeMode.light, ThemeMode.dark, ThemeMode.system],
    defaultValue: ThemeMode.system,
  );
});

final sidebarExtendedStateProvider = StateNotifierProvider<BooleanNotifier, bool>((ref) {
  final box = ref.watch(storageBoxProvider);
  return BooleanNotifier(
    entry: StorageEntry(box: box, key: SettingsEntryKey.sidebarExtended.name),
    defaultValue: true,
  );
});

class _Sidebar extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final tabsRouter = AutoTabsRouter.of(context);
    final isExtended = ref.watch(sidebarExtendedStateProvider);
    return Stack(
      children: [
        NavigationRail(
          extended: isExtended,
          selectedIndex: tabsRouter.activeIndex,
          useIndicator: true,
          destinations: [
            for (final pageLabel in Pages.labels)
              NavigationRailDestination(
                icon: pageLabel.unselectedIcon,
                selectedIcon: pageLabel.selectedIcon,
                label: Text(pageLabel.label),
                padding: EdgeInsets.zero,
              ),
          ],
          onDestinationSelected: (index) => tabsRouter.setActiveIndex(index),
        ),
        Positioned(
          bottom: 0,
          left: 0,
          right: 0,
          child: TextButton(
            child: Icon(isExtended ? Icons.chevron_left : Icons.chevron_right),
            onPressed: () => ref.read(sidebarExtendedStateProvider.notifier).toggle(),
          ),
        ),
      ],
    );
  }
}

class _Drawer extends StatelessWidget {
  const _Drawer({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final tabsRouter = AutoTabsRouter.of(context);
    return Drawer(
      child: ListView(
        children: [
          for (final entry in Pages.labels.asMap().entries)
            ListTile(
              leading: entry.value.unselectedIcon,
              title: Text(entry.value.label),
              onTap: () {
                tabsRouter.setActiveIndex(entry.key);
                context.router.pop();
              },
            )
        ],
      ),
    );
  }
}

class _ResponsiveScaffold extends StatelessWidget {
  final Widget child;

  const _ResponsiveScaffold({
    Key? key,
    required this.child,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final router = AutoTabsRouter.of(context); // router can stay outside of LayoutBuilder.
    return LayoutBuilder(
      builder: (BuildContext context, BoxConstraints constraints) {
        // To keep the state, child has to be placed on the same layer in both layouts.
        final wide = constraints.maxWidth >= 900;
        return Scaffold(
          appBar: wide ? null : AppBar(title: Text(Pages.at(router.activeIndex).label)),
          drawer: wide ? null : const _Drawer(),
          body: NotificationLayer.asSibling(
            child: Row(
              children: [
                if (wide) _Sidebar(),
                Expanded(child: child),
              ],
            ),
          ),
        );
      },
    );
  }
}

class _WindowTitleBar extends StatelessWidget implements PreferredSizeWidget {
  final Widget icon;
  final Widget title;

  const _WindowTitleBar({
    Key? key,
    required this.icon,
    required this.title,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return WindowCaption(
      brightness: theme.brightness,
      backgroundColor: theme.colorScheme.surface,
      title: Container(
        transform: Matrix4.translationValues(-16, 0, 0), // Remove fixed padding in WindowCaption.
        child: Row(
          children: [
            Padding(
              padding: const EdgeInsets.only(left: 4, right: 8),
              child: FittedBox(child: icon),
            ),
            title,
          ],
        ),
      ),
    );
  }

  @override
  Size get preferredSize => const Size.fromHeight(kWindowCaptionHeight);
}

class _WindowFrame extends ConsumerStatefulWidget {
  final Widget child;
  final WindowStateBox _windowStateBox;

  _WindowFrame({Key? key, required this.child})
      : _windowStateBox = WindowStateBox(),
        super(key: key);

  @override
  ConsumerState<_WindowFrame> createState() => _WindowFrameState();
}

class _WindowFrameState extends ConsumerState<_WindowFrame> with WindowListener {
  @override
  void initState() {
    windowManager.addListener(this);
    super.initState();
  }

  @override
  void dispose() {
    windowManager.removeListener(this);
    super.dispose();
  }

  @override
  void onWindowResized() {
    windowManager.getSize().then((size) => widget._windowStateBox.setSize(size));
    super.onWindowResized();
  }

  @override
  void onWindowMoved() {
    windowManager.getPosition().then((offset) => widget._windowStateBox.setOffset(offset));
    super.onWindowMoved();
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.only(top: 0.5),
      child: Scaffold(
        appBar: const _WindowTitleBar(
          icon: Icon(Icons.circle_outlined),
          title: Text('umasagashi'),
        ),
        body: widget.child,
      ),
    );
  }
}

class AppWidget extends StatelessWidget {
  const AppWidget({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final kIsDesktop = {
      TargetPlatform.windows,
      TargetPlatform.linux,
      TargetPlatform.macOS,
    }.contains(defaultTargetPlatform);

    return AutoTabsRouter(
      routes: Pages.routes,
      builder: (context, child, animation) {
        if (!kIsWeb && kIsDesktop) {
          return _WindowFrame(child: _ResponsiveScaffold(child: child));
        } else {
          return _ResponsiveScaffold(child: child);
        }
      },
    );
  }
}

class App extends ConsumerWidget {
  App({Key? key}) : super(key: key);

  final _router = AppRouter();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final themeMode = ref.watch(themeSettingProvider);
    return MaterialApp.router(
      title: 'umasagashi',
      theme: FlexThemeData.light(
        scheme: FlexScheme.blue,
        surfaceMode: FlexSurfaceMode.highScaffoldLowSurface,
        blendLevel: 20,
        appBarOpacity: 0.95,
        subThemesData: const FlexSubThemesData(
          blendOnLevel: 20,
          blendOnColors: false,
          navigationRailMutedUnselectedLabel: false,
          navigationRailMutedUnselectedIcon: false,
          navigationRailLabelType: NavigationRailLabelType.none,
        ),
        visualDensity: FlexColorScheme.comfortablePlatformDensity,
        useMaterial3: true,
        fontFamily: GoogleFonts.notoSans().fontFamily,
      ),
      darkTheme: FlexThemeData.dark(
        scheme: FlexScheme.blue,
        surfaceMode: FlexSurfaceMode.highScaffoldLowSurface,
        blendLevel: 15,
        appBarStyle: FlexAppBarStyle.background,
        appBarOpacity: 0.90,
        subThemesData: const FlexSubThemesData(
          blendOnLevel: 30,
          navigationRailMutedUnselectedLabel: false,
          navigationRailMutedUnselectedIcon: false,
          navigationRailLabelType: NavigationRailLabelType.none,
        ),
        visualDensity: FlexColorScheme.comfortablePlatformDensity,
        useMaterial3: true,
        fontFamily: GoogleFonts.notoSans().fontFamily,
      ),
      themeMode: themeMode,
      routerDelegate: _router.delegate(),
      routeInformationParser: _router.defaultRouteParser(),
    );
  }
}
