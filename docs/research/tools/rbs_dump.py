"""Dump a ReBirth RB-338 song file (.rbs): globals, mixer and effects, the 303 patterns, the 808 and 909
patterns, and a summary of each track's recorded events (pattern changes, knob moves).

    python3 rbs_dump.py SONG.rbs > dump.txt

Exploratory: written on 2026-10-17 to decode KiloMix '98 (tgv-kilomix-98.rbs, from
https://github.com/varyform/re-rebirth). The format (big-endian IFF-style chunks: GLOB, USRI, HEAD, a DEVL
list of device chunks, a TRKL list of TRAK event tracks) was worked out from that file; offsets and flag
meanings are best guesses that matched what the song plays. Don't commit the output for songs you don't own.
"""
import sys, struct, collections, json
data = open(sys.argv[1],'rb').read()
def u8(o): return data[o]
def u32(o): return struct.unpack('>I', data[o:o+4])[0]
def s(o,n): return data[o:o+n].decode('latin1')
def zs(o,n):
    b=data[o:o+n]; i=b.find(b'\0'); b=b if i<0 else b[:i]; return b.decode('cp1252')
def chunks(start,end):
    out=[];o=start
    while o+8<=end:
        i=s(o,4); size=u32(o+4); body=o+8
        if i=='CAT ': out.append({'id':i,'type':s(body,4),'children':chunks(body+4,body+size)})
        else: out.append({'id':i,'at':body,'size':size})
        o=body+size+(size&1)
    return out
top=chunks(12,min(len(data),8+u32(4)))
def find(l,i,t=None):
    for c in l:
        if c['id']==i and (t is None or c.get('type')==t): return c
G=find(top,'GLOB')['at']
print('mode',u8(G),'loop',u8(G+1),'tempo',u32(G+2)/1000,'loopstart bar',u32(G+6)/768,'loopend bar',u32(G+10)/768,'shuffle',u8(G+14),'mod',zs(G+15,65))
U=find(top,'USRI')
if U: print('TITLE',zs(U['at'],41)); print('TEXT',zs(U['at']+41,401)); print('URL',zs(U['at']+442,101))
H=find(top,'HEAD'); print('headver',u8(H['at']+6))
dev=find(top,'CAT ','DEVL')['children']
print('devices',[c['id'] for c in dev])
M=find(dev,'MIXR')['at']
print('master',u8(M),'comp',u8(M+1),'pcf',u8(M+2))
for k,ch in enumerate(['bass1','bass2','r808','r909']):
    b=M+16+k*12; print(' ',ch,'on',u8(b),'lvl',u8(b+1),'pan',u8(b+2),'dly',u8(b+3),'dist',u8(b+4),'pcf',u8(b+5),'comp',u8(b+6))
D=find(dev,'DELY')
if D: o=D['at']; print('DELAY on',u8(o),'steps',u8(o+1),'mode(0=8T,1=16th)',u8(o+2),'fb',u8(o+3),'pan',u8(o+4))
P=find(dev,'PCF ')
if P: o=P['at']; print('PCF on',u8(o),'freq',u8(o+1),'reso',u8(o+2),'amt',u8(o+3),'wave',u8(o+4),'decay',u8(o+5),'mode',u8(o+6))
X=find(dev,'DIST')
if X: o=X['at']; print('DIST on',u8(o),'amt',u8(o+1),'shape',u8(o+2))
C=find(dev,'COMP')
if C: o=C['at']; print('COMP on',u8(o),'amt',u8(o+1),'thr',u8(o+2))
NOTES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B','C^']
basses=[c for c in dev if c['id']=='303 ']
used=collections.defaultdict(set)
for bi,c in enumerate(basses):
    o=c['at']
    print(f'\n303 #{bi+1} on',u8(o),'sel',u8(o+1),'knobs tune/cut/res/env/dec/acc',[u8(o+2+i) for i in range(6)],'wave(1=sq?)',u8(o+8))
    for p in range(32):
        base=o+9+p*34; ln=u8(base+1) or 16; sh=u8(base)
        steps=[]
        nonempty=False
        for st in range(16):
            n=u8(base+2+st*2); f=u8(base+3+st*2)
            if st>=ln: break
            if f&0x10: tok='.'  # rest (or tie if prev slide)
            else:
                nonempty=True
                tok=NOTES[min(12,n)]+('+' if f&4 else '-' if f&8 else '')+('A' if f&2 else '')+('S' if f&1 else '')
            steps.append(tok)
        if nonempty: print(f'  P{p:02d} len{ln} sh{sh}:',' '.join(f'{t:5s}' for t in steps))
R8=['ac','bd','sd','lt','mt','ht','rs','cp','cb','cy','oh','ch']
R9=['ac','bd','sd','lt','mt','ht','rs','cp','ch','oh','cr','rd']
def drums(c,name,nk,tracks):
    o=c['at']; print(f'\n{name} on',u8(o),'sel',u8(o+1),'knobs',[u8(o+2+i) for i in range(nk)])
    head=2+nk+1; stride=2+16*len(tracks)
    for p in range(32):
        base=o+head+p*stride; ln=u8(base+1) or 16
        rows=[]
        for k,t in enumerate(tracks):
            st=[u8(base+2+x*len(tracks)+k) for x in range(16)]
            if any(st): rows.append(f'    {t:3s} '+''.join('.' if v==0 else str(v) for v in st[:ln]))
        if rows: print(f'  P{p:02d} len{ln} sh{u8(base)}'); print('\n'.join(rows))
r8=find(dev,'808 '); r9=find(dev,'909 ')
if r8: drums(r8,'808',27,R8)
if r9: drums(r9,'909',28,R9)
trkl=find(top,'CAT ','TRKL')
def events(c):
    end=c['at']+c['size']; cnt=u32(c['at']); o=c['at']+4; t=0; ev=[]
    for i in range(cnt):
        if o>=end: break
        d=0
        while True:
            b=data[o]; o+=1; d=(d<<7)|(b&0x7f)
            if not (b&0x80) or o>=end: break
        t+=d; ev.append((t,data[o],data[o+1])); o+=2
    return ev
if trkl:
    traks=[c for c in trkl['children'] if c['id']=='TRAK']
    names=['mixer','bass1','bass2','r808','r909','delay','dist','pcf','comp']
    for i,c in enumerate(traks):
        ev=events(c)
        nm=names[i] if i<len(names) else str(i)
        ids=collections.Counter(e[1] for e in ev)
        last=max([e[0] for e in ev],default=0)
        print(f'\nTRACK {nm}: {len(ev)} events, last bar {last/768:.1f}, ctl counts {dict(ids)}')
        if nm in ('bass1','bass2','r808','r909'):
            pat=[(round(t/768,2),v) for (t,cid,v) in ev if cid==1]
            on=[(round(t/768,2),v) for (t,cid,v) in ev if cid==0]
            print('  pattern changes (bar,pat):',pat)
            print('  on/off:',on)
        if nm in ('delay','dist','pcf','comp'):
            print('  ',[(round(t/768,2),cid,v) for (t,cid,v) in ev][:80])
