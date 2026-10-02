"""Block schematics for #02 rev 2. Run: python3 draw.py  (needs schemdraw >= 0.19)
Blocks connect via net tags: S, S̄, G, Q, CAP_BUF, I_ABC, BASE_A, BASE_R."""
import schemdraw
import schemdraw.elements as elm

schemdraw.config(fontsize=11, lw=1.1)


def save(d, name):
    d.save(f'{name}.svg', transparent=False)


def time_channel():
    with schemdraw.Drawing(show=False) as d:
        d += elm.Label().label('Time channel (2×: attack and release)', fontsize=13).at((-6, 4.2))
        op = elm.Opamp(leads=True).label('TL074', 'center', ofst=(-.3, 0), fontsize=9)
        elm.Line().at(op.in2).down(.5)
        elm.Ground()
        sj = elm.Line().at(op.in1).left(1).dot().end
        # feedback
        elm.Line().at(sj).up(1.6)
        elm.Resistor().right().tox(op.out).label('82k')
        elm.Line().down().toy(op.out)
        # pot input
        elm.Resistor().at(sj).left().label('100k')
        p1 = elm.Line().left(.8).end
        pot = elm.Potentiometer().down().anchor('tap').at(p1).reverse().label('B10k\nA / R pot', loc='top')
        elm.Line().at(pot.start).up(.4)
        elm.Ground().up() if False else elm.Label().label('GND (CCW)', loc='top')
        elm.Line().at(pot.end).down(.3)
        elm.Vss().label('−12V (CW)')
        # CV and offset
        elm.Line().at(sj).down(3.4).dot()
        elm.Resistor().left().label('100k', loc='bot')
        elm.Tag().left().label('CV in')
        elm.Line().at(sj).down(3.4)
        elm.Line().down(1.4)
        elm.Resistor().left().label('1.5M', loc='bot')
        elm.Tag().left().label('+12V')
        # base divider
        elm.Line().at(op.out).right(.5)
        elm.Resistor().right().label('15k')
        base = elm.Dot().label('BASE', loc='bottom', ofst=(.45, -.1))
        elm.Resistor().at(base.center).down().label('510Ω', loc='bot')
        elm.Ground()
        elm.Line().at(base.center).up(1.2)
        elm.Tag(width=2.6).up().label('BASE_A (attack)\nBASE_R (release)', loc='rgt', fontsize=9)
        # matched pair
        elm.Line().at(base.center).right(.8)
        q1 = elm.BjtPnp(circle=True).anchor('base').label('Q1', loc='left', ofst=(0, .6))
        elm.Line().at(q1.emitter).right(1.6).dot()
        tail = d.here
        q2 = elm.BjtPnp(circle=True).reverse().anchor('emitter').at(tail).label('Q2', loc='right', ofst=(0, .6))
        elm.Line().at(q2.base).right(.5)
        elm.Ground()
        elm.Line().at(q2.collector).down(.5)
        elm.Ground()
        elm.Resistor().at(tail).up().label('7.5k\ntail')
        elm.Vdd().label('+12V')
        elm.Line().at(q1.collector).down(2.2)
        elm.Tag().down().label('$I_{ABC}$', fontsize=10)
        elm.EncircleBox([q1, q2], padx=.4, pady=.3).linestyle('--').label('BCM857BS (matched)', loc='top', fontsize=9)
    save(d, 'time_channel')


def core():
    with schemdraw.Drawing(show=False) as d:
        elm.Label().label('Core: target, shape, OTA, timing cap, output', fontsize=13).at((-8, 5.5))
        ota = elm.Opamp(leads=True).flip().label('LM13700\n(½)', 'center', ofst=(-.3, 0), fontsize=9)
        elm.Line().at(ota.vs).up(.8)
        elm.Tag().up().label('$I_{ABC}$', fontsize=12)
        # target network
        a = elm.Line().at(ota.in2).left(1.2).dot().end
        t = elm.Line().left(1.8).dot().end
        elm.Resistor().at(t).left().label('39k')
        elm.Tag().left().label('S')
        elm.Resistor().at(t).up().label('56k', loc='bot')
        elm.Vdd().label('+12V')
        elm.Resistor().at(t).down().label('18k', loc='bot')
        elm.Ground()
        elm.Label().at((t[0] - 3.2, t[1] + 2.2)).label('target:\n+4.56V (S high)\n−0.41V (S low)', fontsize=9)
        # shape pot between the OTA inputs; OTA(−) also gets the buffered cap voltage
        nn = elm.Line().at(ota.in1).left(.6).end
        elm.Line().at(a).down().toy(nn[1] - .8)
        elm.ResistorVar().down().label('A5k (log) SHAPE\nCCW = exp\nCW = lin', loc='bot')
        elm.Resistor().down().label('220Ω', loc='bot')
        elm.Line().right().tox(nn)
        corner = elm.Dot().end
        elm.Line().up().toy(nn)
        elm.Resistor().at(corner).right().label('10k', loc='bot')
        elm.Tag(width=2).right().label('CAP_BUF')
        # OTA output = timing cap node, clamp sits above the wire
        elm.Line().at(ota.out).right(1.2)
        cn = elm.Dot().end
        ct = elm.Line().right(3.2).dot().end
        elm.Capacitor().at(ct).down().label('C_T 100nF\nfilm / C0G', loc='top')
        elm.Ground()
        clamp = elm.Opamp(leads=True).right().flip().anchor('in1').at((cn[0] + .3, cn[1] + 1.4)).label('U2B\nclamp', 'center', ofst=(-.3, 0), fontsize=9)
        elm.Line().at(clamp.in1).left(.3)
        elm.Line().down().toy(cn)
        elm.Line().at(clamp.in2).left(.6)
        elm.Ground()
        elm.Line().at(clamp.out).right().tox(ct)
        elm.Diode().down().toy(ct).label('BAS416\n(low leakage)', loc='bot', fontsize=9)
        elm.Dot()
        # buffer U2A
        elm.Line().at(ct).right(1.2)
        buf = elm.Opamp(leads=True).right().anchor('in2').flip().label('U2A', 'center', ofst=(-.3, 0), fontsize=9)
        elm.Line().at(buf.in1).down(.6)
        elm.Line().right().tox(buf.out)
        elm.Line().up().toy(buf.out)
        cb = elm.Line().at(buf.out).right(1).dot().label('CAP_BUF', loc='top', fontsize=9).end
        # output stage U2C, gain 1..2 set by the LEVEL pot
        out = elm.Opamp(leads=True).right().anchor('in2').flip().at(cb).label('U2C\n×1..2', 'center', ofst=(-.3, 0), fontsize=9)
        fb = elm.Line().at(out.in1).down(1).dot().end
        elm.Resistor().at(fb).down().label('10k', loc='bot')
        elm.Ground()
        elm.Line().at(fb).right().tox(out.out)
        elm.ResistorVar().up().toy(out.out).label('B10k\nLEVEL', loc='bot')
        elm.Line().at(out.out).right(.5)
        elm.Resistor().right().label('1k')
        elm.Tag().right().label('OUT')
    save(d, 'core')


def logic():
    with schemdraw.Drawing(show=False) as d:
        elm.Label().label('Logic: gate input, cycle Schmitt, mode, enables, EOC', fontsize=13).at((-6, 4))
        # --- gate comparator U2D ---
        g = elm.Opamp(leads=True).right().flip().label('U2D', 'center', ofst=(-.3, 0), fontsize=9)
        gp = elm.Line().at(g.in2).left(.8).dot().end
        elm.Resistor().left().label('100k')
        elm.Tag(width=1.8).left().label('GATE in')
        elm.Line().at(gp).up(1.2)
        elm.Resistor().right().tox(g.out).label('2.2M')
        elm.Line().down().toy(g.out)
        gr = elm.Line().at(g.in1).left(.8).dot().label('0.96V', loc='bot', ofst=(-.5, -.1), fontsize=9).end
        elm.Resistor().at(gr).left().label('100k', loc='bot')
        elm.Tag(width=1.6).left().label('+12V')
        elm.Resistor().at(gr).down().label('9.1k', loc='bot')
        elm.Ground()
        elm.Line().at(g.out).right(.8).dot()
        elm.Tag().right().label('G')
        elm.Label().at((g.out[0] + .5, g.out[1] - 1.6)).label('on ≈1.5V\noff ≈0.55V', fontsize=9)
        # --- cycle Schmitt U1C ---
        sc = elm.Opamp(leads=True).right().flip().at((0, -8)).anchor('in2').label('U1C', 'center', ofst=(-.3, 0), fontsize=9)
        th = elm.Line().at(sc.in2).left(.8).dot().end
        elm.Resistor().at(th).left().label('47k')
        elm.Ground()
        th2 = elm.Line().at(th).up(1.3).dot().end
        elm.Resistor().left().label('160k')
        elm.Tag(width=1.6).left().label('+12V')
        elm.Line().at(th2).up(1.3)
        elm.Resistor().right().tox(sc.out).label('150k')
        elm.Line().down().toy(sc.out)
        x = elm.Line().at(sc.in1).left(.8).dot().end
        elm.Resistor().at(x).left().label('10k', loc='bot')
        elm.Tag(width=2).left().label('CAP_BUF')
        elm.Line().at(x).down(1.5)
        elm.Resistor().left().label('10k', loc='bot')
        elm.Diode().left().reverse().label('1N4148', loc='bot')
        elm.Switch().left().label('MODE pole B\nclosed in GATE mode', loc='bot', fontsize=9)
        elm.Tag().left().label('G')
        sq = elm.Line().at(sc.out).right(.8).dot().end
        elm.Tag().right().label('Q')
        elm.Label().at((sc.out[0] + 2.2, sc.out[1] + 2.0)).label('C_T thresholds 4.0V / 0.23V\n(OUT 8.0V / 0.46V)', fontsize=9)
        # --- EOC ---
        elm.Line().at(sq).down(2.5)
        elm.Diode().right().label('1N4148')
        eo = elm.Resistor().right().label('1k').end
        elm.Dot()
        elm.Resistor().down().label('100k', loc='bot')
        elm.Ground()
        elm.Line().at(eo).right(.5)
        elm.Tag(width=1.8).right().label('EOC out')
        # --- mode pole A -> S ---
        pa = elm.SwitchSpdt2(action='close').left().at((12, 0)).anchor('a').label('MODE pole A', loc='bot', fontsize=9)
        elm.Line().at(pa.b).left(.6)
        elm.Tag(width=2.2).left().label('G (GATE)')
        elm.Line().at(pa.c).left(.6)
        elm.Tag(width=2.2).left().label('Q (LOOP)')
        sn = elm.Line().at(pa.a).right(1).dot().label('S', loc='top').end
        elm.Line().at(sn).up(1.2)
        elm.Tag().up().label('S', loc='rgt')
        # release kill
        elm.Diode().right().label('1N4148')
        elm.Resistor().right().label('10k')
        elm.Tag(width=1.8).right().label('BASE_R')
        elm.Label().at((sn[0] + 4.5, sn[1] + 2.3)).label('S → core target network\nS high (rise): release off', fontsize=9)
        # inverter -> attack kill
        elm.Line().at(sn).down(4)
        elm.Resistor().right().label('100k')
        qb = elm.Dot().end
        q = elm.BjtNpn(circle=True).right().anchor('base').label('BC847', loc='right', ofst=(.3, -.7), fontsize=9)
        elm.Resistor().at(qb).down().label('22k', loc='top')
        elm.Ground()
        elm.Line().at(q.emitter).down(1)
        elm.Ground()
        sb = elm.Line().at(q.collector).up(.6).dot().end
        elm.Line().up(1)
        elm.Resistor().right().label('4.7k')
        elm.Tag(width=1.6).right().label('+12V')
        elm.Line().at(sb).right(1).label('S̄', loc='bot')
        elm.Diode().right().label('1N4148')
        elm.Resistor().right().label('4.7k')
        elm.Tag(width=1.8).right().label('BASE_A')
        elm.Label().at((sb[0] + 3, sb[1] - 1.2)).label('S low (fall): attack off', fontsize=9)
    save(d, 'logic')


if __name__ == '__main__':
    time_channel()
    core()
    logic()
