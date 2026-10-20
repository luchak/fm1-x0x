"""Summarise a ReBirth .rbs song in 8-bar blocks: which pattern each machine plays, and how busy the
automation is (303 #1 cutoff and resonance, the PCF frequency, 303 #2's knobs).

    python3 rbs_timeline.py SONG.rbs

Exploratory, a companion to rbs_dump.py (same provenance). The song length L is hardcoded for KiloMix
(548 bars); change it for another song.
"""
import sys, struct, collections
data=open(sys.argv[1],'rb').read()
def u32(o): return struct.unpack('>I',data[o:o+4])[0]
def s(o,n): return data[o:o+n].decode('latin1')
def chunks(a,b):
    out=[];o=a
    while o+8<=b:
        i=s(o,4);sz=u32(o+4);body=o+8
        out.append({'id':i,'type':s(body,4),'children':chunks(body+4,body+sz)} if i=='CAT ' else {'id':i,'at':body,'size':sz})
        o=body+sz+(sz&1)
    return out
top=chunks(12,min(len(data),8+u32(4)))
trkl=[c for c in top if c['id']=='CAT ' and c['type']=='TRKL'][0]
traks=[c for c in trkl['children'] if c['id']=='TRAK']
def events(c):
    end=c['at']+c['size'];cnt=u32(c['at']);o=c['at']+4;t=0;ev=[]
    for i in range(cnt):
        if o>=end:break
        d=0
        while True:
            b=data[o];o+=1;d=(d<<7)|(b&0x7f)
            if not(b&0x80) or o>=end:break
        t+=d;ev.append((t/32.0,data[o],data[o+1]));o+=2
    return ev
names=['mixer','bass1','bass2','r808','r909','delay','dist','pcf','comp']
E={names[i]:events(c) for i,c in enumerate(traks)}
# section map: per 8-bar block, which device on and which pattern
state={d:{'on':None,'pat':None} for d in ['bass1','bass2','r808','r909']}
evs=sorted([(t,d,cid,v) for d in state for (t,cid,v) in E[d] if cid in (0,1)])
blocks=collections.OrderedDict()
i=0
L=548
cur={d:{'on':0,'pat':set()} for d in state}
for blk in range(0,L,8):
    pats={d:set() for d in state}; ons={d:set() for d in state}
    while i<len(evs) and evs[i][0]<blk+8:
        t,d,cid,v=evs[i]
        if cid==0: state[d]['on']=v
        else: state[d]['pat']=v
        if state[d]['on']: pats[d].add(state[d]['pat'])
        ons[d].add(state[d]['on'])
        i+=1
    row=[]
    for d in state:
        if state[d]['on'] or 1 in ons[d]:
            p=pats[d] or {state[d]['pat']}
            row.append(f"{d}:{','.join(str(x) for x in sorted(x for x in p if x is not None))}")
        else: row.append(f"{d}:--")
    # automation density bass1 cutoff(3)/reso(4)
    ac=[v for (t,cid,v) in E['bass1'] if cid==3 and blk<=t<blk+8]
    ar=[v for (t,cid,v) in E['bass1'] if cid==4 and blk<=t<blk+8]
    pc=[v for (t,cid,v) in E['pcf'] if cid==1 and blk<=t<blk+8]
    a2=[v for (t,cid,v) in E['bass2'] if cid in(3,4,5,6) and blk<=t<blk+8]
    extra=f" b1cut[{min(ac)}-{max(ac)}]n{len(ac)}" if ac else ''
    extra+=f" b1res[{min(ar)}-{max(ar)}]" if ar else ''
    extra+=f" pcfF[{min(pc)}-{max(pc)}]" if pc else ''
    extra+=f" b2knobs n{len(a2)}" if a2 else ''
    print(f"bar {blk:3d}: "+'  '.join(row)+extra)
