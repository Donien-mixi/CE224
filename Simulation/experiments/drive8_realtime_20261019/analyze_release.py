from pathlib import Path
import numpy as np,json
r=Path(__file__).resolve().parents[3];folder=sorted((r/'.build/rt1_release_check/Simulation/results/runs').glob('*rt1_final'))[-1];rows=[]
for p in sorted(folder.glob('case*_policy1.csv')):
 i=int(p.name[4:6]);a=np.loadtxt(p,delimiter=',',ndmin=2);b=np.loadtxt(folder/f'guard{i:02d}.csv.motion.csv',delimiter=',',ndmin=2);g=np.loadtxt(folder/f'guard{i:02d}.csv',delimiter=',',ndmin=2)
 expected=60 if 69<=i<=72 else 26;t=a[:,0];late=t>expected-2;flags=b[:,3].astype(int);rms=float(np.sqrt(np.mean((a[late,3]*180/np.pi)**2))) if late.any() else None;v_rms=float(np.sqrt(np.mean(a[late,10]**2))) if late.any() else None
 cancelled=bool(np.any(flags&64));j=(i-1)%8;continuous=True;rate=None
 if i<=48 and j>=2:
  cruise=(t>8)&(t<16);br=(b[:,0]>8)&(b[:,0]<16);rate=float(abs(np.mean(a[cruise,17])));ref=float(abs(np.mean(b[br,9])));continuous=rate>.15 and rate>.75*ref
 passed=bool(a[-1,0]>=expected and rms is not None and rms<2 and v_rms<.05 and continuous and (i>=65 or not cancelled))
 if 65<=i<=68:passed=bool(passed and max(abs(b[b[:,0]>15,2]))<.01 and max(abs(b[b[:,0]>15,9]))<.01)
 rows.append(dict(case=i,passed=passed,end=float(t[-1]),expected=expected,cancelled=cancelled,continuous=bool(continuous),yaw_rate=rate,pitch_peak_deg=float(max(abs(a[:,3]*180/np.pi))),speed_peak=float(max(abs(a[:,10]))),tail_pitch_rms_deg=rms,tail_speed_rms=v_rms,fault=int(g[-1,-1])))
out=r/'Simulation/reports/drive8_realtime_20261019';out.mkdir(parents=True,exist_ok=True)
(out/'release_metrics.json').write_text(json.dumps(dict(run=str(folder),passed=sum(x['passed'] for x in rows),total=len(rows),rows=rows),indent=2),encoding='utf-8')
print('Release pass',sum(x['passed'] for x in rows),'/',len(rows));print('Failures',[x for x in rows if not x['passed']])
