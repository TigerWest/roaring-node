import { describe, expect, it } from "vitest";
import { RoaringBitmap64 } from "../../";

describe("RoaringBitmap64.asReadonlyView", () => {
  it("returns the same instance when already frozen", () => {
    const rb = new RoaringBitmap64([1n, 2n]);
    rb.freeze();
    expect(rb.asReadonlyView()).toBe(rb);
  });

  it("creates an independent frozen clone when self is not frozen (Decision B)", () => {
    const rb = new RoaringBitmap64([1n, 2n]);
    const view = rb.asReadonlyView() as unknown as RoaringBitmap64;
    expect(view.isFrozen).toBe(true);
    expect(rb.isFrozen).toBe(false);
    // Mutations on the readonly view throw.
    expect(() => (view as any).add(3n)).toThrow();
    // Mutations on the original do not propagate to the view.
    rb.add(99n);
    expect(view.has(99n)).toBe(false);
    expect(rb.has(99n)).toBe(true);
  });
});
