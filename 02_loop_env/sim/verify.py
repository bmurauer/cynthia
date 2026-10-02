"""Reproduces the design checks from the README. Run: python3 verify.py"""
import numpy as np
from run import sim, cross


def timing_table():
    print("Gate mode: attack 0->8V, release hold->0.3V")
    for shape, name in [(1, 'lin'), (0, 'exp')]:
        for x in [0, 0.5, 0.8, 1.0]:
            est = 0.1e-6 * 4 / (1e-3 * np.exp(-x * 10.2)) * (3.5 if shape == 0 else 1)
            tg = est * 1.6 + 5e-3
            tstop = tg + est * 1.6 + 10e-3
            r = sim(tstop, tstop / 20000, TG=tg, XA=x, XR=x, SHAPE=shape)
            ta = cross(r['t'], r['out'], 8.0)
            tr = cross(r['t'], r['out'], 0.3, rising=False, after=tg)
            fa = f"{(ta - 5e-3) * 1e3:9.2f} ms" if ta else "      n/a"
            fr = f"{(tr - tg - 5e-3) * 1e3:9.2f} ms" if tr else "      n/a (sim too short)"
            print(f"  pot {x:4} {name}: attack {fa}   release {fr}")


def cv_table():
    print("Attack CV at pot 50%, linear")
    for cv in [-5, -1, 0, 1, 5]:
        est = 0.1e-6 * 4 / (1e-3 * np.exp(-0.5 * 10.4)) * np.exp(-cv * 0.8)
        tg = est * 1.5 + 5e-3
        r = sim(tg + est * 1.5 + 5e-3, (tg + est * 1.5 + 5e-3) / 20000, TG=tg, SHAPE=1, CVA=cv)
        ta = cross(r['t'], r['out'], 8.0)
        print(f"  CV {cv:+} V: attack {(ta - 5e-3) * 1e3:8.2f} ms")


def loop_corners():
    print("Loop mode worst-case corners (OTA offset, rails, op-amp swing)")
    stalls = 0
    for shape in [0, 1]:
        for x in [0, 0.5]:
            for vos in [-4e-3, 4e-3]:
                for vp in [11.0, 11.5, 12.0]:
                    for headroom in [1.6, 2.2]:
                        est = 0.1e-6 * 4 / (1e-3 * np.exp(-x * 10.2)) * (3.5 if shape == 0 else 1) * 2
                        tstop = est * 8
                        r = sim(tstop, tstop / 40000, TG=1e3, TP=2e3, MODE=1, XA=x, XR=x,
                                SHAPE=shape, VOS=vos, VP=vp, SAT=vp - headroom)
                        t, q, o = r['t'], r['q'], r['out']
                        edges = np.where((q[:-1] < 0) & (q[1:] >= 0))[0]
                        ok = len(edges) >= 3
                        stalls += not ok
                        m = t > t[edges[1]] if ok else t > 0
                        print(f"  shape={shape} pot={x} Vos={vos * 1e3:+.0f}mV rails={vp} "
                              f"swing={vp - headroom:.1f}: out {o[m].min():.2f}..{o[m].max():.2f} V "
                              f"{'' if ok else 'STALLED'}")
    print(f"  stalls: {stalls}")


def shape_plot(path='shapes.png'):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(9, 5))
    for shape in [0, 0.25, 0.5, 0.75, 1.0]:
        r = sim(1.0, 5e-5, TG=0.45, XA=0.5, XR=0.5, SHAPE=shape)
        ax.plot(r['t'] * 1e3, r['out'], label=f"shape pot {shape:.0%}")
    ax.set_xlabel('time [ms]')
    ax.set_ylabel('output [V]')
    ax.set_title('Gate mode, attack/release pots at 50%, shape pot CCW (exp) .. CW (lin)')
    ax.legend()
    ax.grid(alpha=.3)
    fig.savefig(path, dpi=90)
    print(f"wrote {path}")


def timing_plot(path='timing.png'):
    """Gate mode and loop mode: gate, stage S, Q/EOC, cap and output on one time axis."""
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    runs = [
        ('Gate mode (attack pot 50%, release pot 55%, shape 50%)',
         sim(0.30, 5e-5, TG=0.12, TP=1, XA=0.5, XR=0.55, SHAPE=0.5)),
        ('Loop mode, free-running: gate input ignored (same pots)',
         sim(0.30, 5e-5, TG=1, TP=2, MODE=1, XA=0.5, XR=0.55, SHAPE=0.5)),
    ]
    fig, axes = plt.subplots(4, 2, figsize=(12, 7), sharex=True,
                             gridspec_kw=dict(height_ratios=[1, 1, 1, 2.5]))
    for col, (title, r) in enumerate(runs):
        t = r['t'] * 1e3
        rows = [('GATE in', [(r['gate'], 'gate jack')]),
                ('S (stage)', [(r['s'], 'S: high = rise')]),
                ('Q / EOC', [(r['q'], 'Q (EOC = Q, clipped to 0..9V)')]),
                ('volts', [(r['out'], 'OUT'), (r['ct'], 'C_T (core)')])]
        for row, (ylabel, traces) in enumerate(rows):
            ax = axes[row, col]
            for y, lbl in traces:
                ax.plot(t, y, label=lbl)
            ax.set_ylabel(ylabel)
            ax.grid(alpha=.3)
            ax.legend(loc='upper right', fontsize=8)
        axes[0, col].set_title(title)
        axes[3, col].axhline(8.0, color='grey', lw=.6, ls='--')
        axes[3, col].axhline(0.45, color='grey', lw=.6, ls='--')
        axes[3, col].set_xlabel('time [ms]')
    fig.tight_layout()
    fig.savefig(path, dpi=90)
    print(f"wrote {path}")


if __name__ == '__main__':
    timing_table()
    cv_table()
    loop_corners()
    shape_plot()
    timing_plot()
