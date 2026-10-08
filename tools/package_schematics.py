"""Filter PCB/PANEL documents from an exported EasyEDA epro2 copy, never live files."""
import argparse,collections,json,pathlib,zipfile
def package(source,target):
 with zipfile.ZipFile(source) as archive:
  names=[name for name in archive.namelist() if name.endswith('.epru')]
  if len(names)!=1:raise ValueError('Expected exactly one epru source')
  blocks=[];current=[]
  for line in archive.read(names[0]).decode('utf-8').splitlines(keepends=True):
   if line.startswith('{"type":"DOCHEAD"'):
    if current:blocks.append(''.join(current))
    current=[line]
   else:current.append(line)
  if current:blocks.append(''.join(current))
  kept=[];types=collections.Counter()
  for block in blocks:
   if not block.strip():continue
   head=json.loads(block.splitlines()[0].split('||',1)[1].rstrip('|'))
   if head['docType'] in ('PCB','PANEL'):continue
   kept.append(block);types[head['docType']]+=1
  if not types['SCH_PAGE']:raise ValueError('No schematic pages')
  target=pathlib.Path(target);target.parent.mkdir(parents=True,exist_ok=True)
  with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as output:
   for name in archive.namelist():
    if name.startswith('IMAGE/'):continue # Discard cached editor previews.
    output.writestr(name,''.join(kept) if name==names[0] else archive.read(name))
  return dict(types)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('target');args=p.parse_args()
 print(json.dumps(package(args.source,args.target),ensure_ascii=False))
