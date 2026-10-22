from pathlib import Path
import json
r=Path(__file__).resolve().parents[3];out=r/'Simulation/reports/drive8_realtime_20261019';p=Path(json.loads((out/'release_metrics.json').read_text())['run'])
s=f'''a=csvRead("{(p/'case29_policy1.csv').as_posix()}",",");b=csvRead("{(p/'guard29.csv.motion.csv').as_posix()}",",");
f=scf();f.figure_size=[1050 900];
subplot(3,1,1);plot(a(:,1),a(:,7)*180/%pi);xgrid();ylabel("Pitch CF deg");title("RT1 forward-left 0.72m/s, held3..17s (conditional model)");
subplot(3,1,2);plot(a(:,1),a(:,17)*180/%pi,b(:,1),b(:,9)*180/%pi);xgrid();ylabel("Unwrapped yaw deg");legend("Plant","Reference");
subplot(3,1,3);plot(a(:,1),a(:,18),b(:,1),b(:,10));xgrid();ylabel("Yaw rate rad/s");xlabel("Time s");legend("Plant","Reference");xs2png(f,"{(out/'continuous_diagonal.png').as_posix()}");
dt=diff(a(:,1));v=a(:,3);psi=a(:,17);xx=[0;cumsum(dt.*(v(1:$-1).*cos(psi(1:$-1))+v(2:$).*cos(psi(2:$)))/2)];yy=[0;cumsum(dt.*(v(1:$-1).*sin(psi(1:$-1))+v(2:$).*sin(psi(2:$)))/2)];
f=scf();f.figure_size=[850 800];plot(xx,yy);xgrid();axes=gca();axes.isoview="on";xlabel("World X m");ylabel("World Y m");title("Conditional flat-floor path: continuous forward-left then release");xs2png(f,"{(out/'continuous_path.png').as_posix()}");
a=csvRead("{(p/'case27_policy1.csv').as_posix()}",",");b=csvRead("{(p/'guard27.csv.motion.csv').as_posix()}",",");f=scf();f.figure_size=[1050 800];subplot(2,1,1);plot(a(:,1),a(:,17)*180/%pi);xgrid();ylabel("Unwrapped yaw deg");title("RT1 fast pure turn: multiple continuous revolutions");subplot(2,1,2);plot(a(:,1),a(:,18),b(:,1),b(:,10));xgrid();ylabel("Yaw rate rad/s");xlabel("Time s");legend("Plant","Reference");xs2png(f,"{(out/'fast_pure_turn.png').as_posix()}");quit;
'''
(r/'.build/plot_rt1_release.sce').write_text(s,encoding='utf-8')
print(r/'.build/plot_rt1_release.sce')
